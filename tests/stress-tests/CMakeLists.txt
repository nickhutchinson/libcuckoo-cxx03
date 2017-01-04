add_executable(stress_checked stress_checked.cc)
add_executable(stress_unchecked stress_unchecked.cc)

# Test with smaller table size in debug configuration.
set(table_power "$<$<CONFIG:Debug>:21>$<$<NOT:$<CONFIG:Debug>>:24>")

add_test(NAME stress_checked
    COMMAND stress_checked --power "${table_power}")
add_test(NAME stress_unchecked
    COMMAND stress_unchecked --power "${table_power}")
