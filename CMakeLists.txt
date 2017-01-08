cmake_minimum_required(VERSION 3.1.0)
project(libcuckoo LANGUAGES CXX)
set (libcuckoo_VERSION_MAJOR 0)
set (libcuckoo_VERSION_MINOR 2)

if(MSVC)
  add_compile_options(
    -wd4503  # Suppress: 'X': decorated name length exceeded, name was truncated
  )
else()
  set(CMAKE_CXX_STANDARD 11)
  add_compile_options(
    -Wall
    -Wextra
  )
endif()

# Add the libcuckoo interface target
add_subdirectory(libcuckoo)

# Build examples
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Build tests -- this only builds tests that were specified
enable_testing()
add_subdirectory(tests)
