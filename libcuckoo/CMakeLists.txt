add_library(libcuckoo INTERFACE)

# Include relative to the base directory
target_include_directories(libcuckoo INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Add debug logging in debug mode
target_compile_definitions(libcuckoo INTERFACE
    $<$<CONFIG:Debug>:LIBCUCKOO_DEBUG=1>)

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
