#include <iostream>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include "ast.h"

using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule = llvm::make_unique<Module>("jit", TheContext);
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const char *Str) {
    fprintf(stderr, "LogError: %s\n", Str);
    return nullptr;
}

Value *NumberExprAST::codegen() {
    return ConstantFP::get(TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
    // Look this variable up in the function.
    Value *V = NamedValues[Name];
    if (!V)
        LogErrorV("Unknown variable name");
    return V;
}

Value *BinaryExprAST::codegen() {
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;

    switch (Op) {
        case '+':
            return Builder.CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder.CreateFSub(L, R, "subtmp");
        case '*':
            return Builder.CreateFMul(L, R, "multmp");
        case '<':
            L = Builder.CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext),
                                        "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

Value *CallExprAST::codegen() {
    // Look up the name in the global module table.
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen() {
    // Make the function type:  double(double,double) etc.
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));

    FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Function *FunctionAST::codegen() {
    Function *TheFunction = Proto->codegen();

    if (!TheFunction)
        return nullptr;

    Function *ExistingFunction = TheModule->getFunction(Proto->getName());

    if (ExistingFunction) {
        if (!ExistingFunction->empty())
            return (Function *) LogErrorV("Function cannot be redefined.");

        // Validate that definition arguments match previous 'extern' declaration.
        if (ExistingFunction->arg_size() != TheFunction->arg_size())
            return (Function *) LogErrorV("Function definition and extern declaration argument mismatch.");
    }

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);e
    Builder.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[Arg.getName()] = &Arg;

    if (Value *RetVal = Body->codegen()) {
        // Finish off the function.
        Builder.CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        if (verifyFunction(*TheFunction, &llvm::errs())) {
            // Error generating code, remove function.
            TheFunction->eraseFromParent();
            return nullptr;
        }

        return TheFunction;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();
    return nullptr;
}
