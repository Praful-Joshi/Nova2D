#pragma once
#include <spdlog/spdlog.h>

namespace Nova {

// Initialise once at engine startup — call before any LOG_ macros
void initLogger();

}  // namespace Nova

// ── Logging macros ────────────────────────────────────────────────────────────
// In Release builds (NDEBUG), LOG_TRACE and LOG_DEBUG compile to nothing.
// LOG_INFO and above always compile in.

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)