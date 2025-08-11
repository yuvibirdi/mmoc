# Find LLVM and configure linking
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include dirs / defs (SYSTEM to suppress warnings)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Prefer modern imported targets. Components we need:
set(_MMOC_LLVM_COMPONENTS Core IRReader BitWriter Target Support)
set(LLVM_LIBS "")
foreach(_comp IN LISTS _MMOC_LLVM_COMPONENTS)
    if(TARGET LLVM::${_comp})
        list(APPEND LLVM_LIBS LLVM::${_comp})
    endif()
endforeach()

# If individual component targets not available (e.g. Arch ships monolithic lib), fallback
if(LLVM_LIBS STREQUAL "")
    if(TARGET LLVM) # Some distributions export a single target named LLVM
        set(LLVM_LIBS LLVM)
    else()
        # Last resort: map component names (may fail if libs not shipped separately)
        llvm_map_components_to_libnames(LLVM_LIBS_TEMP
            core
            irreader
            bitwriter
            target
            support
        )
        set(LLVM_LIBS ${LLVM_LIBS_TEMP})
    endif()
endif()

message(STATUS "LLVM libraries: ${LLVM_LIBS}")
