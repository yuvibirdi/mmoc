# Find LLVM and configure linking
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set LLVM configuration
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Find the LLVM libraries we need
llvm_map_components_to_libnames(LLVM_LIBS 
    support 
    core 
    irreader 
    bitwriter 
    target 
    x86asmparser 
    x86codegen
    aarch64asmparser
    aarch64codegen
)

message(STATUS "LLVM libraries: ${LLVM_LIBS}")
