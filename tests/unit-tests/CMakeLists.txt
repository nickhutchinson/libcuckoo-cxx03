add_executable(test_runner
    test_bracket_operator.cc
    test_constructor.cc
    test_hash_properties.cc
    test_heterogeneous_compare.cc
    test_iterator.cc
    test_maximum_hashpower.cc
    test_minimum_load_factor.cc
    test_noncopyable_types.cc
    test_resize.cc
    test_runner.cc
    test_user_exceptions.cc
    unit_test_util.cc
)

add_test(NAME test_runner COMMAND test_runner)
