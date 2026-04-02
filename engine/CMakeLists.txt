add_library(nova2d_engine STATIC
    core/Logger.cpp
    core/Clock.cpp
    core/Window.cpp
    core/Engine.cpp
)

# Detect Homebrew prefix (handles both Intel /usr/local and Apple Silicon /opt/homebrew)
execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE HOMEBREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

target_include_directories(nova2d_engine
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
            $<$<BOOL:${HOMEBREW_PREFIX}>:${HOMEBREW_PREFIX}/include>
)

target_link_libraries(nova2d_engine
    PUBLIC  SDL2::SDL2
    PRIVATE spdlog::spdlog_header_only   # ← header-only avoids DLL linkage conflict
)

set_project_warnings(nova2d_engine)