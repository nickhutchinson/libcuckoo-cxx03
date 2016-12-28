cmake_minimum_required(VERSION 3.4)
project(libcuckoo LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 98)
add_definitions(-DCATCH_CONFIG_NO_CPP11)

if (MSVC)
  string(REGEX REPLACE "[-/]W[1-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4")
  add_definitions(
    -DBOOST_ALL_NO_LIB
    -D_CRT_SECURE_NO_WARNINGS
    -D_SCL_SECURE_NO_WARNINGS
    -D_UNICODE
    -DUNICODE
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
  )
  add_compile_options(
    -wd4127 # Suppress: conditional expression is constant
    -wd4324 # Suppress: structure was padded due to alignment specifier
    -wd4456 # Suppress: declaration of 'x' hides previous local declaration
    -wd4459 # Suppress: declaration of 'x' hides global declaration
    -wd4510 # Suppress: default constructor could not be generated
    -wd4512 # Suppress: assignment operator could not be generated
  )

  # Enable debug symbols in release builds
  add_compile_options($<$<CONFIG:Release>:-Zi>)
  string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE " -DEBUG -OPT:REF")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -DEBUG -OPT:REF")
  string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE    " -DEBUG -OPT:REF")

else ()
  add_compile_options(
    -Wall
    -Wextra

    # Enable debug symbols in release builds
    $<$<CONFIG:Release>:-g>
  )
  if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
    add_compile_options(-fcolor-diagnostics)
  endif ()
endif ()

find_package(Boost REQUIRED COMPONENTS atomic chrono random system thread)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES})
add_definitions(-DBOOST_THREAD_VERSION=4)

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:LIBCUCKOO_DEBUG=1>)

if (CMAKE_SYSTEM_NAME MATCHES Linux)
  link_libraries(-pthread -lrt)
endif ()

add_library(libcuckoo INTERFACE)
target_include_directories(libcuckoo INTERFACE src)

add_subdirectory(examples)
add_subdirectory(tests)
