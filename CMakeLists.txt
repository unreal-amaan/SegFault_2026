cmake_minimum_required(VERSION 3.20)

project(CompilerCostModel LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake from: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})

separate_arguments(
    LLVM_DEFINITIONS_LIST
    NATIVE_COMMAND
    ${LLVM_DEFINITIONS}
)

add_definitions(${LLVM_DEFINITIONS_LIST})

include(AddLLVM)

add_subdirectory(llvm/passes)