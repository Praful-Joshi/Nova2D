#ifndef TAG_ARGS_HPP
#define TAG_ARGS_HPP

#include <string>

#include "tag_peer.hpp"

// nova2d_tag's command line, e.g. `nova2d_tag --id 2 --players 3 --peer 1=192.168.1.10`
struct TagArgs {
    TagConfig config;
    bool helpRequested = false;
    std::string error;  // why the command line is invalid; empty if it's valid
};

TagArgs parseTagArgs(int argc, const char* const argv[]);

std::string tagUsage();

#endif
