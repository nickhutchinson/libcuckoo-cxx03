add_executable(hellohash hellohash.cc)
add_executable(count_freq count_freq.cc)
add_executable(nested_table nested_table.cc)

add_test(NAME hellohash COMMAND hellohash)
add_test(NAME count_freq COMMAND count_freq)
add_test(NAME nested_table COMMAND nested_table)
