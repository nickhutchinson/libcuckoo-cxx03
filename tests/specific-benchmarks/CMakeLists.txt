add_executable(insert_throughput insert_throughput.cc)
target_link_libraries(insert_throughput
    PRIVATE test_util
    PRIVATE libcuckoo
)

add_executable(read_throughput read_throughput.cc)
target_link_libraries(read_throughput
    PRIVATE test_util
    PRIVATE libcuckoo
)

add_executable(read_insert_throughput read_insert_throughput.cc)
target_link_libraries(read_insert_throughput
    PRIVATE test_util
    PRIVATE libcuckoo
)

add_optimized_cxx_flags(insert_throughput read_throughput read_insert_throughput)
