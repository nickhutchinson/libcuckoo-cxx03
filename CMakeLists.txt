cmake_minimum_required(VERSION 3.1.0)
project(libcuckoo LANGUAGES CXX)
set(libcuckoo_VERSION_MAJOR 0)
set(libcuckoo_VERSION_MINOR 2)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Default build type" FORCE)
endif()

# Enable CTest
include(CTest)

# Enable C++11 for all targets
set(CMAKE_CXX_STANDARD 11)

if (MSVC)
  # Use -W4 (default is -W3)
  string(REGEX REPLACE "[-/]W[1-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4")

  # Suppress pedantic warnings we don't care about.
  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -D_SCL_SECURE_NO_WARNINGS
  )
  add_compile_options(
    -wd4127 # Suppress: conditional expression is constant
    -wd4324 # Suppress: 'x': structure was padded due to alignment specifier
    -wd4456 # Suppress: declaration of 'x' hides previous local declaration
    -wd4459 # Suppress: declaration of 'x' hides global declaration
    -wd4503 # Suppress: 'x': decorated name length exceeded, name was truncated
    -wd4510 # Suppress: default constructor could not be generated
    -wd4512 # Suppress: assignment operator could not be generated
    -wd4610 # Suppress: object 'class' can never be instantiated - user-defined constructor required
  )

  # Enable debug symbols in release builds
  add_compile_options($<$<CONFIG:Release>:-Zi>)
  string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE " -DEBUG -OPT:REF")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -DEBUG -OPT:REF")
  string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE    " -DEBUG -OPT:REF")

else ()
  # GCC-compatible compiler.
  add_compile_options(
    -Wall
    -Wextra

    # Enable debug symbols in release builds
    $<$<CONFIG:Release>:-g>
  )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES Clang AND CMAKE_GENERATOR MATCHES Ninja)
  # Force color diagnostics.
  add_compile_options(-fcolor-diagnostics)
endif ()

# Add the libcuckoo interface target
add_subdirectory(libcuckoo)

# Build examples
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Build tests -- this only builds tests that were specified
add_subdirectory(tests)
