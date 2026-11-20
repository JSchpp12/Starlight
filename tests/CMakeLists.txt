find_package(GTest CONFIG REQUIRED)

add_executable(starlight_tests
    test_OneTimeCommandBufferSynchronizer.cpp
)

target_link_libraries(starlight_tests PRIVATE
    starlight
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(starlight_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(starlight_tests)
