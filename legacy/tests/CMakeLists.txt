add_executable(nova2d_legacy_tests
    test_physics.cpp
)

target_link_libraries(nova2d_legacy_tests
    PRIVATE
        nova2d_tag_core
        Catch2::Catch2WithMain
)

set_project_warnings(nova2d_legacy_tests)

include(CTest)
include(Catch)
catch_discover_tests(nova2d_legacy_tests)
