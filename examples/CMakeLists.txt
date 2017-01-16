add_executable(nested_table nested_table.cc)
target_link_libraries(nested_table libcuckoo)

add_executable(hellohash hellohash.cc)
target_link_libraries(hellohash libcuckoo)

add_executable(count_freq count_freq.cc)
target_link_libraries(count_freq libcuckoo)

add_test(NAME nested_table COMMAND nested_table)
add_test(NAME hellohash COMMAND hellohash)
add_test(NAME count_freq COMMAND count_freq)
