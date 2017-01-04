add_executable(read_throughput read_throughput.cc)
add_executable(read_insert_throughput read_insert_throughput.cc)
add_executable(insert_throughput insert_throughput.cc)

# Test with smaller table size in debug configuration.
set(table_power "$<$<CONFIG:Debug>:21>$<$<NOT:$<CONFIG:Debug>>:25>")

add_test(NAME insert_throughput
  COMMAND insert_throughput --table-capacity 1 --power "${table_power}")
add_test(NAME read_throughput
  COMMAND read_throughput --power "${table_power}")
add_test(NAME read_insert_throughput
  COMMAND read_insert_throughput --table-capacity 1 --power "${table_power}")
