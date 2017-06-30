cmake_minimum_required(VERSION 3.7)
project(Kaleidoscope)

set(CMAKE_CXX_STANDARD 11)

message("system: ${CMAKE_SYSTEM_PREFIX_PATH}")

list(APPEND CMAKE_PREFIX_PATH /usr/local/Cellar/llvm/4.0.0_1/)
find_library(LLVM_LIBRARY llvm)
find_path(LLVM_INCLUDE_DIR llvm)
message("llvm lib: ${LLVM_LIBRARY}")
message("llvm include: ${LLVM_INCLUDE_DIR}")

set(SOURCE_FILES main.cpp lexer.cpp lexer.h ast.cpp ast.h parser.cpp parser.h)
add_executable(Kaleidoscope ${SOURCE_FILES})
target_link_libraries(Kaleidoscope ${LLVM_LIBRARY})
include_directories(${LLVM_INCLUDE_DIR})