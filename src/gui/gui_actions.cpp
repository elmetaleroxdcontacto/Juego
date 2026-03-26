#include "gui/gui_internal.h"
#include "gui/gui_audio.h"

#ifdef _WIN32

#include "engine/game_settings.h"

#include <sstream>

namespace gui_win32 {

namespace {

bool containsText(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

std::wstring buildValidationDialogText(const ValidationSuiteSummary& summary) {
    std::ostringstream out;
    out << "Resumen de validacion\n\n";
    out << "Fallos logicos: " << summary.logicFailureCount << "\n";
    out << "Errores de datos: " << summary.dataErrorCount << "\n";
    out << "Advertencias de datos: " << summary.dataWarningCount << "\n";

    int shown = 0;
    for (const auto& line : summary.lines) {
        if (!(containsText(line, "[FAIL]") || containsText(line, "[ERROR]") || containsText(line, "[WARNING]"))) {
            continue;
        }
        if (shown == 0) out << "\nIncidencias destacadas:";
        if (shown >= 6) {
            out << "\n- ... ver reporte completo en saves/roster_validation_report.txt";
            break;
        }
        out << "\n- " << line;
        ++shown;
    }

    if (shown == 0) {
        out << "\n\nNo se detectaron incidencias activas.";
    }
    out << "\n\nDetalle completo disponible en saves/roster_validation_report.txt";
    return utf8ToWide(out.str());
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

void pulseFrontendTiming(AppState& state) {
    const int delay = game_settings::pageTransitionDelayMs(state.settings);
    if (delay <= 0) return;
    UpdateWindow(state.window);
    Sleep(static_cast<DWORD>(delay));
}

void persistSettings(AppState& state) {
    game_settings::saveToDisk(state.settings);
}

}  // namespace

void startNewCareer(AppState& state) {
    if (state.career.divisions.empty()) {
        MessageBoxW(state.window, L"No se encontraron divisiones disponibles.", L"Football Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    syncManagerNameFromUi(state);
    if (!check_game_ready(state)) {
        refreshCurrentPage(state);
        if (state.gameSetup.division.empty()) {
            SetFocus(state.divisionCombo);
        } else if (state.gameSetup.club.empty()) {
            SetFocus(state.teamCombo);
        } else {
            SetFocus(state.managerEdit);
        }
        setStatus(state, state.gameSetup.inlineMessage + (state.gameSetup.managerError.empty() ? std::string() : " " + state.gameSetup.managerError));
        return;
    }

    ServiceResult result = startCareerService(state.career,
                                              state.gameSetup.division,
                                              state.gameSetup.club,
                                              state.gameSetup.manager);
    if (result.ok) {
        game_settings::applyNewCareerDifficulty(state.career, state.settings);
        result.messages.push_back("Configuracion aplicada: " + game_settings::settingsSummary(state.settings) + ".");
    }
    if (!result.ok) {
        std::string message = result.messages.empty() ? "No se pudo iniciar la carrera." : result.messages.front();
        MessageBoxW(state.window, utf8ToWide(message).c_str(), L"Football Manager", MB_OK | MB_ICONWARNING);
    }
    syncCombosFromCareer(state);
    state.selectedPlayerName.clear();
    state.selectedTransferPlayer.clear();
    setCurrentPage(state, GuiPage::Dashboard);
    setStatus(state, result.messages.empty()
                         ? (result.ok ? "Nueva carrera iniciada." : "No se pudo iniciar la carrera.")
                         : result.messages.back());
    if (!result.messages.empty() && result.messages.size() > 1) {
        showLoadMessagesCompact(state, result, "Nueva carrera");
    }
}

void continueCareer(AppState& state) {
    if (state.career.myTeam) {
        pulseFrontendTiming(state);
        queuePageTransition(state, GuiPage::Dashboard);
        setStatus(state, "Carrera activa retomada desde memoria.");
        return;
    }
    loadCareer(state);
}

void loadCareer(AppState& state) {
    pulseFrontendTiming(state);
    ServiceResult result = loadCareerService(state.career);
    if (!result.ok) {
        std::string message = result.messages.empty() ? "No se encontro una carrera guardada." : result.messages.front();
        MessageBoxW(state.window, utf8ToWide(message).c_str(), L"Football Manager", MB_OK | MB_ICONINFORMATION);
        setStatus(state, message);
        fillDivisionCombo(state, state.gameSetup.division);
        fillTeamCombo(state, state.gameSetup.division, state.gameSetup.club);
        refreshAll(state);
        return;
    }

    syncCombosFromCareer(state);
    state.selectedPlayerName.clear();
    state.selectedTransferPlayer.clear();
    queuePageTransition(state, GuiPage::Dashboard);
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
    setStatus(state,
              "Simulando semana en modo " +
                  game_settings::simulationModeLabel(state.settings.simulationMode) +
                  " a velocidad " + game_settings::simulationSpeedLabel(state.settings.simulationSpeed) + "...");
    UpdateWindow(state.window);
    pulseFrontendTiming(state);
    ServiceResult result = simulateCareerWeekService(state.career);
    syncCombosFromCareer(state);
    refreshAll(state);
    setStatus(state, result.messages.empty() ? "Semana simulada." : result.messages.back());
}

void validateSystem(AppState& state) {
    setStatus(state, "Ejecutando auditoria de datos...");
    UpdateWindow(state.window);
    ValidationSuiteSummary summary = runValidationService();
    const std::wstring dialogText = buildValidationDialogText(summary);
    if (summary.ok) {
        MessageBoxW(state.window, dialogText.c_str(), L"Football Manager", MB_OK | MB_ICONINFORMATION);
        setStatus(state, "Auditoria completada sin fallas.");
    } else {
        MessageBoxW(state.window, dialogText.c_str(), L"Football Manager", MB_OK | MB_ICONWARNING);
        setStatus(state, "Auditoria completada con fallas.");
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

void openFrontendMenu(AppState& state) {
    pulseFrontendTiming(state);
    setCurrentPage(state, GuiPage::MainMenu);
    setStatus(state,
              state.career.myTeam
                  ? "Volviste al menu principal. Usa Continuar para retomar la carrera activa."
                  : "Menu principal listo. Entra a Jugar o revisa Configuraciones.");
    if (state.menuContinueButton && IsWindowEnabled(state.menuContinueButton)) SetFocus(state.menuContinueButton);
    else if (state.menuPlayButton) SetFocus(state.menuPlayButton);
}

void openSettingsMenu(AppState& state) {
    pulseFrontendTiming(state);
    setCurrentPage(state, GuiPage::Settings);
    setStatus(state, "Configuraciones abiertas. Ajusta frontend, accesibilidad y audio del menu.");
    if (state.menuVolumeButton) SetFocus(state.menuVolumeButton);
}

void openCreditsPage(AppState& state) {
    pulseFrontendTiming(state);
    setCurrentPage(state, GuiPage::Credits);
    setStatus(state, "Creditos abiertos. La portada mantiene la identidad del manager game.");
    if (state.menuBackButton) SetFocus(state.menuBackButton);
}

void cycleFrontendVolume(AppState& state) {
    game_settings::cycleVolume(state.settings);
    refreshMenuMusicVolume(state);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Volumen ajustado a " + game_settings::volumeLabel(state.settings.volume) + ".");
}

void cycleFrontendDifficulty(AppState& state) {
    game_settings::cycleDifficulty(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Dificultad actual: " + game_settings::difficultyLabel(state.settings.difficulty) + ".");
}

void cycleFrontendSimulationSpeed(AppState& state) {
    game_settings::cycleSimulationSpeed(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Velocidad actual: " + game_settings::simulationSpeedLabel(state.settings.simulationSpeed) + ".");
}

void cycleFrontendSimulationMode(AppState& state) {
    game_settings::cycleSimulationMode(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Modo de simulacion actual: " + game_settings::simulationModeLabel(state.settings.simulationMode) + ".");
}

void cycleFrontendLanguage(AppState& state) {
    game_settings::cycleLanguage(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Idioma actual: " + game_settings::languageLabel(state.settings.language) + ".");
}

void cycleFrontendTextSpeed(AppState& state) {
    game_settings::cycleTextSpeed(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Velocidad de texto: " + game_settings::textSpeedLabel(state.settings.textSpeed) + ".");
}

void cycleFrontendVisualProfile(AppState& state) {
    game_settings::cycleVisualProfile(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Perfil visual: " + game_settings::visualProfileLabel(state.settings.visualProfile) + ".");
}

void cycleFrontendMenuMusicMode(AppState& state) {
    game_settings::cycleMenuMusicMode(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Musica del frontend: " + game_settings::menuMusicModeLabel(state.settings.menuMusicMode) + ".");
}

void toggleFrontendAudioFade(AppState& state) {
    game_settings::toggleMenuAudioFade(state.settings);
    persistSettings(state);
    refreshCurrentPage(state);
    setStatus(state, "Audio del menu: " + game_settings::menuAudioFadeLabel(state.settings.menuAudioFade) + ".");
}

}  // namespace gui_win32

#endif
