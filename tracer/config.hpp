#pragma once

#include <vector>
#include <string>

static constexpr auto file_filter = "filters.txt";
static constexpr auto file_hooks  = "hooks.txt";
static constexpr auto file_trace  = "trace.txt";

/// Load list of modules that will be logged.
///
std::vector<std::string> load_filters();

/// Load list of dlls that will be hooked.
///
std::vector<std::string> load_hooks();
