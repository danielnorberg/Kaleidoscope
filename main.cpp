#include "parser.h"
#include "KaleidoscopeJIT.h"

#include "llvm/Support/TargetSelect.h"
#include "ast.h"

int main() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    TheJIT = llvm::make_unique<llvm::orc::KaleidoscopeJIT>();

    InitializeModuleAndPassManager();
    ParserMainLoop();
    return 0;
}
