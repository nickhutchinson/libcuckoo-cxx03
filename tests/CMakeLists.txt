include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(sys/resource.h HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILE_CXX(mach/mach.h HAVE_MACH_MACH_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(test_util INTERFACE)
target_include_directories(test_util INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
if (CMAKE_SYSTEM_NAME MATCHES Windows)
    # https://msdn.microsoft.com/en-us/library/windows/desktop/ms683219(v=vs.85).aspx
    target_compile_definitions(test_util INTERFACE PSAPI_VERSION=2)
endif ()

# Adds optimized flags to a list of executables
function(add_optimized_cxx_flags)
    foreach(target ${ARGN})
        if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR
            ((CMAKE_CXX_COMPILER_ID MATCHES "GNU")))
            target_compile_options(${target} PUBLIC -O3 -DNDEBUG)
        endif()
    endforeach()
endfunction()

# Adds debug flags to a list of executables
function(add_debug_cxx_flags)
    foreach(target ${ARGN})
        if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR
            ((CMAKE_CXX_COMPILER_ID MATCHES "GNU")))
            target_compile_options(${target} PUBLIC -O0 -g)
        endif()
    endforeach()
endfunction()

# Add catch for unit tests
if (BUILD_TESTS OR BUILD_UNIT_TESTS)
    add_subdirectory(Catch)
    add_subdirectory(unit-tests)
endif()

if (BUILD_TESTS OR BUILD_STRESS_TESTS)
    add_subdirectory(stress-tests)
endif()

if (BUILD_TESTS OR BUILD_SPECIFIC_BENCHMARKS)
    add_subdirectory(specific-benchmarks)
endif()

if (BUILD_TESTS OR BUILD_UNIVERSAL_BENCHMARK)
    add_subdirectory(universal-benchmark)
endif()
