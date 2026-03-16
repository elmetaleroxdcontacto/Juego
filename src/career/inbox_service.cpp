#include "career/inbox_service.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {

string extractChannel(const string& entry, bool scouting) {
    if (scouting) return "Scouting";
    if (entry.size() > 3 && entry[0] == '[') {
        size_t end = entry.find(']');
        if (end != string::npos && end > 1) return entry.substr(1, end - 1);
    }
    return "Inbox";
}

}  // namespace

namespace inbox_service {

vector<InboxEntry> buildCombinedInbox(const Career& career, size_t limit) {
    vector<InboxEntry> out;
    size_t managerStart = career.managerInbox.size() > limit ? career.managerInbox.size() - limit : 0;
    for (size_t i = managerStart; i < career.managerInbox.size(); ++i) {
        const string& entry = career.managerInbox[i];
        out.push_back({extractChannel(entry, false), entry, false});
    }
    size_t scoutStart = career.scoutInbox.size() > limit / 2 ? career.scoutInbox.size() - limit / 2 : 0;
    for (size_t i = scoutStart; i < career.scoutInbox.size(); ++i) {
        const string& entry = career.scoutInbox[i];
        out.push_back({extractChannel(entry, true), entry, true});
    }
    if (out.size() > limit) {
        out.erase(out.begin(), out.begin() + static_cast<long long>(out.size() - limit));
    }
    return out;
}

vector<string> buildInboxSummaryLines(const Career& career, size_t limit) {
    vector<string> lines;
    const auto entries = buildCombinedInbox(career, limit);
    for (const auto& entry : entries) {
        lines.push_back(entry.channel + " | " + entry.text);
    }
    if (lines.empty()) lines.push_back("Inbox limpio: no hay alertas ni informes nuevos.");
    return lines;
}

string buildInboxDigest(const Career& career, size_t limit) {
    const auto lines = buildInboxSummaryLines(career, limit);
    ostringstream out;
    out << "Centro del manager\r\n";
    for (const auto& line : lines) out << "- " << line << "\r\n";
    return out.str();
}

}  // namespace inbox_service
