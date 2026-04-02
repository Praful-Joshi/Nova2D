add_executable(nova2d_tests
    math/test_vec2.cpp
)

target_link_libraries(nova2d_tests
    PRIVATE
        nova2d_engine
        Catch2::Catch2WithMain
)

set_project_warnings(nova2d_tests)

include(CTest)
include(Catch)
catch_discover_tests(nova2d_tests)