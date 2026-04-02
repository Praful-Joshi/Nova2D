function(set_project_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /wd4100
            /utf-8      # ← fixes the Windows fmt/spdlog unicode error
        >
    )
endfunction()