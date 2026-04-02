#include "core/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Nova {

void initLogger() {
    auto console = spdlog::stdout_color_mt("nova2d");
    spdlog::set_default_logger(console);

#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);  // Release: info and above
#else
    spdlog::set_level(spdlog::level::trace);  // Debug: everything
#endif

    spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
}

}  // namespace Nova