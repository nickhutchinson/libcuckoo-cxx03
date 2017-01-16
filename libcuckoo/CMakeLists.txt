add_library(libcuckoo INTERFACE)

# Include relative to the base directory
target_include_directories(libcuckoo INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Enable threading for all targets that link with libcuckoo
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(libcuckoo INTERFACE Threads::Threads)

install(
FILES
    city_hasher.hh
    cuckoohash_config.hh
    cuckoohash_map.hh
    cuckoohash_util.hh
    lazy_array.hh
DESTINATION
    ${CMAKE_INSTALL_PREFIX}/include/libcuckoo
)
