#include "career/inbox_service.h"

#include "career/manager_advice.h"
#include "career/staff_service.h"
#include "career/transfer_briefing.h"
#include "utils/utils.h"

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

void pushUniqueLine(vector<string>& lines, const string& line) {
    if (line.empty()) return;
    if (find(lines.begin(), lines.end(), line) == lines.end()) {
        lines.push_back(line);
    }
}

string priorityLabel(int urgency) {
    if (urgency >= 72) return "Urgente";
    if (urgency >= 48) return "Atencion";
    return "Seguimiento";
}

int inferredUrgency(const string& text, bool scouting) {
    const string lower = toLower(text);
    int urgency = scouting ? 42 : 35;
    if (lower.find("urgente") != string::npos || lower.find("critica") != string::npos ||
        lower.find("bajo presion") != string::npos) {
        urgency += 24;
    }
    if (lower.find("lesion") != string::npos || lower.find("fatiga") != string::npos ||
        lower.find("fisico") != string::npos || lower.find("carga") != string::npos) {
        urgency += 28;
    }
    if (lower.find("vestuario") != string::npos || lower.find("moral") != string::npos ||
        lower.find("promesa") != string::npos || lower.find("charla") != string::npos) {
        urgency += 24;
    }
    if (lower.find("directiva") != string::npos || lower.find("objetivo") != string::npos ||
        lower.find("confianza") != string::npos) {
        urgency += 22;
    }
    if (lower.find("contrato") != string::npos || lower.find("renov") != string::npos) {
        urgency += 20;
    }
    if (lower.find("deuda") != string::npos || lower.find("caja") != string::npos ||
        lower.find("presupuesto") != string::npos || lower.find("salario") != string::npos ||
        lower.find("finanz") != string::npos) {
        urgency += 20;
    }
    if (lower.find("mercado") != string::npos || lower.find("shortlist") != string::npos ||
        lower.find("scouting") != string::npos || lower.find("fich") != string::npos) {
        urgency += 16;
    }
    if (lower.find("rival") != string::npos || lower.find("partido") != string::npos ||
        lower.find("tact") != string::npos || lower.find("instruccion") != string::npos) {
        urgency += 14;
    }
    if (lower.find("estable") != string::npos || lower.find("sin alertas") != string::npos) {
        urgency -= 18;
    }
    return clampInt(urgency, 10, 96);
}

string inferDestination(const string& text, bool scouting) {
    if (scouting) return "Fichajes";
    const string lower = toLower(text);
    if (lower.find("lesion") != string::npos || lower.find("fatiga") != string::npos ||
        lower.find("fisico") != string::npos || lower.find("carga") != string::npos ||
        lower.find("moral") != string::npos || lower.find("vestuario") != string::npos ||
        lower.find("promesa") != string::npos || lower.find("plantel") != string::npos) {
        return "Plantilla";
    }
    if (lower.find("contrato") != string::npos || lower.find("renov") != string::npos ||
        lower.find("mercado") != string::npos || lower.find("shortlist") != string::npos ||
        lower.find("scouting") != string::npos || lower.find("fich") != string::npos) {
        return "Fichajes";
    }
    if (lower.find("deuda") != string::npos || lower.find("caja") != string::npos ||
        lower.find("presupuesto") != string::npos || lower.find("salario") != string::npos ||
        lower.find("finanz") != string::npos) {
        return "Finanzas";
    }
    if (lower.find("directiva") != string::npos || lower.find("objetivo") != string::npos ||
        lower.find("confianza") != string::npos || lower.find("staff") != string::npos) {
        return "Directiva";
    }
    if (lower.find("rival") != string::npos || lower.find("partido") != string::npos ||
        lower.find("tact") != string::npos || lower.find("instruccion") != string::npos ||
        lower.find("presion") != string::npos) {
        return "Tacticas";
    }
    return "Inicio";
}

string inferCommand(const string& text, bool scouting) {
    if (scouting) return "Actualizar informe";
    const string lower = toLower(text);
    if (lower.find("lesion") != string::npos || lower.find("medico") != string::npos) return "Revisar medico";
    if (lower.find("fatiga") != string::npos || lower.find("fisico") != string::npos ||
        lower.find("carga") != string::npos || lower.find("recuperacion") != string::npos) {
        return "Gestionar carga";
    }
    if (lower.find("moral") != string::npos || lower.find("vestuario") != string::npos ||
        lower.find("promesa") != string::npos || lower.find("charla") != string::npos) {
        return "Reunion/charla";
    }
    if (lower.find("contrato") != string::npos || lower.find("renov") != string::npos) return "Revisar contrato";
    if (lower.find("mercado") != string::npos || lower.find("shortlist") != string::npos ||
        lower.find("scouting") != string::npos || lower.find("fich") != string::npos) {
        return "Abrir mercado";
    }
    if (lower.find("deuda") != string::npos || lower.find("caja") != string::npos ||
        lower.find("presupuesto") != string::npos || lower.find("salario") != string::npos ||
        lower.find("finanz") != string::npos) {
        return "Revisar caja";
    }
    if (lower.find("directiva") != string::npos || lower.find("objetivo") != string::npos ||
        lower.find("confianza") != string::npos) {
        return "Revisar objetivo";
    }
    if (lower.find("rival") != string::npos || lower.find("partido") != string::npos ||
        lower.find("tact") != string::npos || lower.find("instruccion") != string::npos) {
        return "Preparar partido";
    }
    return "Revisar";
}

inbox_service::ActionableInboxEntry makeAction(const string& channel,
                                               const string& text,
                                               bool scouting,
                                               int urgencyOverride = -1) {
    inbox_service::ActionableInboxEntry entry;
    entry.channel = channel.empty() ? "Centro" : channel;
    entry.text = text;
    entry.scouting = scouting;
    entry.urgency = urgencyOverride >= 0 ? clampInt(urgencyOverride, 10, 96) : inferredUrgency(text, scouting);
    entry.priority = priorityLabel(entry.urgency);
    entry.destination = inferDestination(channel + " " + text, scouting);
    entry.command = inferCommand(channel + " " + text, scouting);
    return entry;
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

vector<ActionableInboxEntry> buildActionableInbox(const Career& career, size_t limit) {
    vector<ActionableInboxEntry> out;
    if (limit == 0) return out;

    auto push = [&](const ActionableInboxEntry& entry) {
        if (entry.text.empty()) return;
        const string key = entry.channel + "|" + entry.text;
        const bool exists = any_of(out.begin(), out.end(), [&](const ActionableInboxEntry& current) {
            return current.channel + "|" + current.text == key;
        });
        if (!exists) out.push_back(entry);
    };

    if (career.myTeam) {
        for (const auto& recommendation : staff_service::buildStaffRecommendations(career, limit)) {
            push(makeAction("Staff",
                            recommendation.staffRole + " [" + recommendation.severity + "] " +
                                recommendation.summary + " Accion: " + recommendation.suggestedAction,
                            false,
                            recommendation.urgency));
        }
        for (const auto& line : manager_advice::buildManagerActionLines(career, limit)) {
            push(makeAction("Agenda", line, false));
        }
        for (const auto& line : transfer_briefing::buildMarketPulseLines(career, max<size_t>(2, limit / 2))) {
            push(makeAction("Mercado", line, false));
        }
    }

    for (const auto& entry : buildCombinedInbox(career, limit)) {
        push(makeAction(entry.channel, entry.text, entry.scouting));
    }

    sort(out.begin(), out.end(), [](const ActionableInboxEntry& left, const ActionableInboxEntry& right) {
        if (left.urgency != right.urgency) return left.urgency > right.urgency;
        if (left.channel != right.channel) return left.channel < right.channel;
        return left.text < right.text;
    });
    if (out.size() > limit) {
        const vector<ActionableInboxEntry> sorted = out;
        vector<ActionableInboxEntry> selected(sorted.begin(), sorted.begin() + static_cast<long long>(limit));
        auto hasChannel = [](const vector<ActionableInboxEntry>& entries, const string& channel) {
            return any_of(entries.begin(), entries.end(), [&](const ActionableInboxEntry& entry) {
                return entry.channel == channel;
            });
        };
        auto sameEntry = [](const ActionableInboxEntry& left, const ActionableInboxEntry& right) {
            return left.channel == right.channel && left.text == right.text;
        };
        auto ensureChannel = [&](const string& channel) {
            if (limit < 3 || hasChannel(selected, channel)) return;
            auto candidate = find_if(sorted.begin(), sorted.end(), [&](const ActionableInboxEntry& entry) {
                return entry.channel == channel;
            });
            if (candidate == sorted.end()) return;
            auto duplicate = find_if(selected.begin(), selected.end(), [&](const ActionableInboxEntry& entry) {
                return sameEntry(entry, *candidate);
            });
            if (duplicate != selected.end()) return;
            auto replace = selected.end();
            for (auto it = selected.end(); it != selected.begin();) {
                --it;
                if (it->channel != "Staff" && it->channel != "Agenda" && it->channel != "Mercado") {
                    replace = it;
                    break;
                }
            }
            if (replace == selected.end()) {
                replace = min_element(selected.begin(), selected.end(), [](const ActionableInboxEntry& left,
                                                                           const ActionableInboxEntry& right) {
                    if (left.urgency != right.urgency) return left.urgency < right.urgency;
                    return left.text > right.text;
                });
            }
            if (replace == selected.end()) return;
            *replace = *candidate;
            sort(selected.begin(), selected.end(), [](const ActionableInboxEntry& left, const ActionableInboxEntry& right) {
                if (left.urgency != right.urgency) return left.urgency > right.urgency;
                if (left.channel != right.channel) return left.channel < right.channel;
                return left.text < right.text;
            });
        };
        ensureChannel("Staff");
        ensureChannel("Agenda");
        ensureChannel("Mercado");
        out = selected;
    }
    if (out.empty()) {
        out.push_back(makeAction("Centro",
                                 "Inbox limpio: no hay alertas ni informes nuevos.",
                                 false,
                                 12));
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

vector<string> buildPriorityInboxLines(const Career& career, size_t limit) {
    vector<string> lines;
    for (const auto& entry : buildActionableInbox(career, limit)) {
        pushUniqueLine(lines,
                       entry.channel + " | " + entry.priority + " | " +
                           entry.destination + " -> " + entry.command + " | " + entry.text);
    }

    if (lines.empty()) lines.push_back("Centro | Inbox limpio: no hay alertas ni informes nuevos.");
    if (lines.size() > limit) lines.resize(limit);
    return lines;
}

string buildManagerHubDigest(const Career& career, size_t limit) {
    const auto lines = buildPriorityInboxLines(career, limit);
    ostringstream out;
    out << "Centro del manager\r\n";
    for (const auto& line : lines) out << "- " << line << "\r\n";
    return out.str();
}

}  // namespace inbox_service
