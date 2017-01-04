add_executable(stress_checked stress_checked.cc)
target_link_libraries(stress_checked
    PRIVATE test_util
    PRIVATE libcuckoo
)

add_executable(stress_unchecked stress_unchecked.cc)
target_link_libraries(stress_unchecked
    PRIVATE test_util
    PRIVATE libcuckoo
)

add_optimized_cxx_flags(stress_checked stress_unchecked)

# Test with smaller table size in debug configuration to avoid timeouts on
# Appveyor.
set(table_power "$<$<CONFIG:Debug>:21>$<$<NOT:$<CONFIG:Debug>>:24>")

add_test(NAME stress_checked
    COMMAND stress_checked --power "${table_power}")
add_test(NAME stress_unchecked
    COMMAND stress_unchecked --power "${table_power}")
