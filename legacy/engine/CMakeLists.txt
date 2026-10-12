add_library(nova2d_legacy_engine STATIC
    src/core/entity.cpp
    src/core/eventmanager.cpp
    src/core/game.cpp
    src/core/init.cpp
    src/core/input.cpp
    src/core/timeline.cpp
    src/handlers/collision_handler.cpp
    src/handlers/control_handler.cpp
    src/handlers/death_handler.cpp
    src/handlers/sideboundary_handler.cpp
    src/modules/collisions.cpp
    src/modules/control.cpp
    src/modules/deathzone.cpp
    src/modules/gravity.cpp
    src/modules/movement.cpp
    src/modules/platform.cpp
    src/modules/sideboundary.cpp
    src/modules/spawnpoint.cpp
    src/networking/peer.cpp
    src/rendering/render.cpp
    src/rendering/scaling.cpp
)

target_include_directories(nova2d_legacy_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(nova2d_legacy_engine
    PUBLIC
        SDL2::SDL2
        cppzmq
        Boost::headers
        Threads::Threads
)

set_project_warnings(nova2d_legacy_engine)
