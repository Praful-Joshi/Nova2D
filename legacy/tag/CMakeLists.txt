# Tag's game logic is a library so the legacy tests can build headless Peers
# exactly the way nova2d_tag does.
add_library(nova2d_tag_core STATIC
    arena.cpp
    minimap.cpp
    tag_peer.cpp
)

target_include_directories(nova2d_tag_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(nova2d_tag_core PUBLIC nova2d_legacy_engine)
set_project_warnings(nova2d_tag_core)

add_executable(nova2d_tag main.cpp)
target_link_libraries(nova2d_tag
    PRIVATE
        nova2d_tag_core
        $<$<BOOL:${WIN32}>:SDL2::SDL2main>
)
set_project_warnings(nova2d_tag)
