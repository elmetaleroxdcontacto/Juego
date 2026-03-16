#pragma once

#include "engine/models.h"

#include <cstddef>
#include <string>
#include <vector>

namespace inbox_service {

struct InboxEntry {
    std::string channel;
    std::string text;
    bool scouting = false;
};

std::vector<InboxEntry> buildCombinedInbox(const Career& career, std::size_t limit = 12);
std::vector<std::string> buildInboxSummaryLines(const Career& career, std::size_t limit = 8);
std::string buildInboxDigest(const Career& career, std::size_t limit = 6);

}  // namespace inbox_service
