add_executable(nova2d_game main.cpp)

target_link_libraries(nova2d_game
    PRIVATE
        nova2d_engine
        $<$<BOOL:${WIN32}>:SDL2::SDL2main>   # ← required on Windows for WinMain
)

set_project_warnings(nova2d_game)

# Copy SDL2 DLLs next to executable on Windows
if(WIN32)
    add_custom_command(TARGET nova2d_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:nova2d_game>
    )
endif()