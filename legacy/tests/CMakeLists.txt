add_executable(nova2d_legacy_tests
    test_abilities.cpp
    test_camera.cpp
    test_deathzone.cpp
    test_physics.cpp
    test_text.cpp
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
