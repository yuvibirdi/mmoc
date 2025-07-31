# Find LLVM and configure linking
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set LLVM configuration
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})  # Use SYSTEM to suppress warnings
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Find the LLVM libraries we need - use INTERFACE library to avoid duplicates
llvm_map_components_to_libnames(LLVM_LIBS_TEMP 
    core 
    irreader 
    bitwriter 
    target
    support    # Needed for Host.h
)

# Create an INTERFACE library to avoid duplicate linking
add_library(llvm_interface INTERFACE)
target_link_libraries(llvm_interface INTERFACE ${LLVM_LIBS_TEMP})

# Use the INTERFACE library instead of raw LLVM_LIBS
set(LLVM_LIBS llvm_interface)

message(STATUS "LLVM libraries: ${LLVM_LIBS}")
