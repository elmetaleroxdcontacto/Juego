#include "gui/gui_internal.h"

#ifdef _WIN32

#include <sstream>

namespace gui_win32 {

namespace {

bool containsText(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

void showServiceMessages(AppState& state, const ServiceResult& result, const std::string& title) {
    if (result.messages.empty()) return;
    std::ostringstream out;
    for (size_t i = 0; i < result.messages.size(); ++i) {
        if (i) out << "\n";
        out << result.messages[i];
    }
    MessageBoxW(state.window,
                utf8ToWide(out.str()).c_str(),
                utf8ToWide(title).c_str(),
                MB_OK | (result.ok ? MB_ICONINFORMATION : MB_ICONWARNING));
}

void showLoadMessagesCompact(AppState& state, const ServiceResult& result, const std::string& title) {
    if (result.messages.empty()) return;

    int generatedTemporarySquads = 0;
    int warningLines = 0;
    std::string auditLine;
    std::vector<std::string> highlights;

    for (const auto& message : result.messages) {
        if (containsText(message, "se genera una base temporal para")) {
            generatedTemporarySquads++;
            continue;
        }
        if (containsText(message, "Errores: ") && containsText(message, "Advertencias:")) {
            auditLine = message;
            continue;
        }
        if (containsText(message, "roster_validation_report.txt")) {
            continue;
        }
        if (containsText(message, "[WARNING]") || containsText(message, "[ERROR]")) {
            warningLines++;
            if (highlights.size() < 3) highlights.push_back(message);
        }
    }

    const bool hasHardAuditErrors = containsText(auditLine, "Errores: ") && !containsText(auditLine, "Errores: 0");
    const bool hasUsefulSummary = generatedTemporarySquads > 0 || !auditLine.empty() || !highlights.empty();
    if (!hasUsefulSummary) return;
    if (generatedTemporarySquads == 0 && !hasHardAuditErrors) return;

    std::ostringstream out;
    out << result.messages.front();

    if (generatedTemporarySquads > 0) {
        out << "\n\nPlantillas temporales generadas: " << generatedTemporarySquads << ".";
    }
    if (!auditLine.empty()) {
        out << "\n" << auditLine;
    }
    if (!highlights.empty()) {
        out << "\n\nIncidencias destacadas:";
        for (const auto& line : highlights) {
            out << "\n- " << line;
        }
    }
    if (warningLines > static_cast<int>(highlights.size())) {
        out << "\n- ... y " << (warningLines - static_cast<int>(highlights.size())) << " incidencia(s) adicional(es).";
    }
    out << "\n\nDetalle completo disponible en saves/roster_validation_report.txt";

    MessageBoxW(state.window,
                utf8ToWide(out.str()).c_str(),
                utf8ToWide(title).c_str(),
                MB_OK | MB_ICONINFORMATION);
}

void finalizeAction(AppState& state,
                    const ServiceResult& result,
                    const std::string& title,
                    bool forceDialog = false) {
    if (result.ok) {
        syncCombosFromCareer(state);
        refreshAll(state);
    }
    if (!result.messages.empty()) {
        setStatus(state, result.messages.back());
    }
    if (!result.ok || forceDialog || result.messages.size() > 2) {
        showServiceMessages(state, result, title);
    }
}

}  // namespace

void startNewCareer(AppState& state) {
    if (state.career.divisions.empty()) {
        MessageBoxW(state.window, L"No se encontraron divisiones disponibles.", L"Football Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string divisionId = selectedDivisionId(state);
    if (divisionId.empty()) divisionId = state.career.divisions.front().id;
    std::string teamName = comboText(state.teamCombo);
    syncManagerNameFromUi(state);
    ServiceResult result = startCareerService(state.career, divisionId, teamName, state.career.managerName);
    if (!result.ok) {
        std::string message = result.messages.empty() ? "No se pudo iniciar la carrera." : result.messages.front();
        MessageBoxW(state.window, utf8ToWide(message).c_str(), L"Football Manager", MB_OK | MB_ICONWARNING);
    }
    syncCombosFromCareer(state);
    state.selectedPlayerName.clear();
    state.selectedTransferPlayer.clear();
    setCurrentPage(state, GuiPage::Dashboard);
    setStatus(state, result.messages.empty() ? "Nueva carrera iniciada." : result.messages.back());
    if (!result.messages.empty() && result.messages.size() > 1) {
        showLoadMessagesCompact(state, result, "Nueva carrera");
    }
}

void loadCareer(AppState& state) {
    ServiceResult result = loadCareerService(state.career);
    if (!result.ok) {
        std::string message = result.messages.empty() ? "No se encontro una carrera guardada." : result.messages.front();
        MessageBoxW(state.window, utf8ToWide(message).c_str(), L"Football Manager", MB_OK | MB_ICONINFORMATION);
        setStatus(state, message);
        fillDivisionCombo(state);
        fillTeamCombo(state, selectedDivisionId(state));
        refreshAll(state);
        return;
    }

    syncCombosFromCareer(state);
    state.selectedPlayerName.clear();
    state.selectedTransferPlayer.clear();
    setCurrentPage(state, GuiPage::Dashboard);
    setStatus(state, result.messages.empty() ? "Carrera cargada." : result.messages.back());
    if (!result.messages.empty() && result.messages.size() > 1) {
        showLoadMessagesCompact(state, result, "Carga de carrera");
    }
}

void saveCareer(AppState& state) {
    if (!state.career.myTeam) return;
    syncManagerNameFromUi(state);
    ServiceResult result = saveCareerService(state.career);
    refreshAll(state);
    setStatus(state, result.messages.empty() ? "Carrera guardada." : result.messages.back());
}

void simulateWeek(AppState& state) {
    if (!state.career.myTeam) return;
    syncManagerNameFromUi(state);
    setStatus(state, "Simulando semana...");
    UpdateWindow(state.window);
    ServiceResult result = simulateCareerWeekService(state.career);
    syncCombosFromCareer(state);
    refreshAll(state);
    setStatus(state, result.messages.empty() ? "Semana simulada." : result.messages.back());
}

void validateSystem(AppState& state) {
    setStatus(state, "Ejecutando validacion...");
    UpdateWindow(state.window);
    ValidationSuiteSummary summary = runValidationService();
    if (summary.ok) {
        MessageBoxW(state.window, L"La suite de validacion termino sin fallas.", L"Football Manager", MB_OK | MB_ICONINFORMATION);
        setStatus(state, "Validacion completada sin fallas.");
    } else {
        MessageBoxW(state.window, L"La suite de validacion detecto fallas.", L"Football Manager", MB_OK | MB_ICONWARNING);
        setStatus(state, "Validacion completada con fallas.");
    }
}

void runScoutingAction(AppState& state) {
    ServiceResult result = state.currentPage == GuiPage::News
        ? createScoutingAssignmentService(state.career, "", "", 3)
        : scoutPlayersService(state.career, "Todas", "");
    finalizeAction(state, result, "Scouting", true);
}

void runBuyAction(AppState& state) {
    if (state.currentPage != GuiPage::Transfers) return;
    int row = selectedListViewRow(state.squadList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un objetivo del mercado.", L"Mercado", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = buyTransferTargetService(state.career,
                                                    listViewText(state.squadList, row, 10),
                                                    listViewText(state.squadList, row, 0));
    finalizeAction(state, result, "Fichaje");
}

void runPreContractAction(AppState& state) {
    if (state.currentPage != GuiPage::Transfers) return;
    int row = selectedListViewRow(state.squadList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un objetivo del mercado.", L"Precontrato", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = signPreContractService(state.career,
                                                  listViewText(state.squadList, row, 10),
                                                  listViewText(state.squadList, row, 0));
    finalizeAction(state, result, "Precontrato");
}

void runRenewAction(AppState& state) {
    int row = selectedListViewRow(state.squadList);
    if (row < 0 || (state.currentPage != GuiPage::Squad && state.currentPage != GuiPage::Youth && state.currentPage != GuiPage::Finances)) {
        MessageBoxW(state.window, L"Selecciona un jugador.", L"Renovar", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = renewPlayerContractService(state.career, listViewText(state.squadList, row, 0));
    finalizeAction(state, result, "Renovacion");
}

void runSellAction(AppState& state) {
    int row = selectedListViewRow(state.squadList);
    if (row < 0 || state.currentPage == GuiPage::Transfers) {
        MessageBoxW(state.window, L"Selecciona un jugador de tu plantilla.", L"Venta", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = sellPlayerService(state.career, listViewText(state.squadList, row, 0));
    finalizeAction(state, result, "Venta");
}

void runPlanAction(AppState& state) {
    int row = selectedListViewRow(state.squadList);
    if (row < 0 || (state.currentPage != GuiPage::Squad && state.currentPage != GuiPage::Youth)) {
        MessageBoxW(state.window, L"Selecciona un jugador.", L"Plan individual", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = state.currentPage == GuiPage::Squad
        ? cyclePlayerInstructionService(state.career, listViewText(state.squadList, row, 0))
        : cyclePlayerDevelopmentPlanService(state.career, listViewText(state.squadList, row, 0));
    finalizeAction(state, result, state.currentPage == GuiPage::Squad ? "Instruccion individual" : "Plan individual");
}

void runInstructionAction(AppState& state) {
    if (state.currentPage == GuiPage::Dashboard) {
        finalizeAction(state, holdTeamMeetingService(state.career), "Reunion");
        return;
    }
    if (state.currentPage == GuiPage::Squad || state.currentPage == GuiPage::Youth) {
        int row = selectedListViewRow(state.squadList);
        if (row < 0) {
            MessageBoxW(state.window, L"Selecciona un jugador.", L"Charla", MB_OK | MB_ICONINFORMATION);
            return;
        }
        finalizeAction(state, talkToPlayerService(state.career, listViewText(state.squadList, row, 0)), "Charla");
        return;
    }
    finalizeAction(state, cycleMatchInstructionService(state.career), "Instruccion");
}

void runShortlistAction(AppState& state) {
    if (state.currentPage != GuiPage::Transfers) return;
    int row = selectedListViewRow(state.squadList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un objetivo del mercado.", L"Shortlist", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = shortlistPlayerService(state.career,
                                                  listViewText(state.squadList, row, 10),
                                                  listViewText(state.squadList, row, 0));
    finalizeAction(state, result, "Shortlist");
}

void runFollowShortlistAction(AppState& state) {
    ServiceResult result = followShortlistService(state.career);
    finalizeAction(state, result, "Seguimiento", true);
}

void runUpgradeAction(AppState& state, ClubUpgrade upgrade, const std::string& title) {
    ServiceResult result = upgradeClubService(state.career, upgrade);
    finalizeAction(state, result, title);
}

}  // namespace gui_win32

#endif
