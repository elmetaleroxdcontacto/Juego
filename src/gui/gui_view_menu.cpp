#include "gui/gui_view_builders.h"

#ifdef _WIN32

#include "engine/game_settings.h"

#include <sstream>

namespace gui_win32 {

GuiPageModel buildMainMenuModel(AppState& state) {
    GuiPageModel model;
    model.title = game_settings::gameTitle();
    model.breadcrumb = "Inicio > Menu principal";
    model.infoLine = "Portada principal del manager. La experiencia comparte frontend entre GUI y consola sin romper el flujo real.";
    model.summary.title = "FrontMenuOverview";
    model.detail.title = "FrontMenuProfile";
    model.feed.title = "FrontMenuRoadmap";
    model.metrics = {
        {"Perfil", "Portada", kThemeAccentBlue},
        {"Dificultad", game_settings::difficultyLabel(state.settings.difficulty), kThemeAccent},
        {"Velocidad", game_settings::simulationSpeedLabel(state.settings.simulationSpeed), kThemeWarning},
        {"Modo", game_settings::simulationModeLabel(state.settings.simulationMode), kThemeAccentBlue},
        {"Audio", game_settings::volumeLabel(state.settings.volume), kThemeAccentGreen}
    };

    model.summary.content =
        "Panorama de arranque\r\n\r\n"
        "Jugar abre el flujo real del proyecto: dashboard, club, carrera, mercado, tacticas y noticias.\r\n\r\n"
        "Configuraciones abre la cabina inicial para dejar lista la partida antes de entrar.\r\n\r\n"
        "La portada esta pensada como base escalable para seguir creciendo hacia continuar, cargar, creditos y perfil.";

    std::ostringstream detail;
    detail << "Perfil del manager\r\n\r\n";
    detail << "Volumen: " << game_settings::volumeLabel(state.settings.volume) << "\r\n";
    detail << "Dificultad: " << game_settings::difficultyLabel(state.settings.difficulty) << "\r\n";
    detail << game_settings::difficultyDescription(state.settings.difficulty) << "\r\n\r\n";
    detail << "Velocidad de simulacion: " << game_settings::simulationSpeedLabel(state.settings.simulationSpeed) << "\r\n";
    detail << game_settings::simulationSpeedDescription(state.settings.simulationSpeed) << "\r\n\r\n";
    detail << "Modo de simulacion: " << game_settings::simulationModeLabel(state.settings.simulationMode) << "\r\n";
    detail << game_settings::simulationModeDescription(state.settings.simulationMode) << "\r\n\r\n";
    detail << "Preparado para abrir el centro del club sin perder la configuracion actual.";
    model.detail.content = detail.str();

    model.feed.lines = {
        "Estado actual: frontend listo para una experiencia tipo manager game.",
        "Navegacion GUI: Tab, Enter, flechas y Esc desde configuraciones.",
        "Preparado para futuro: Continuar, Cargar, Creditos y salir al escritorio.",
        "F11 sigue disponible para alternar modos de pantalla en cualquier momento."
    };
    return model;
}

GuiPageModel buildSettingsPageModel(AppState& state) {
    GuiPageModel model;
    model.title = "Configuraciones";
    model.breadcrumb = "Inicio > Configuraciones";
    model.infoLine = "Cabina inicial de ajustes. Cambia el tono del juego antes de entrar al centro del club.";
    model.summary.title = "SettingsDesk";
    model.detail.title = "SettingsImpact";
    model.feed.title = "SettingsReturn";
    model.metrics = {
        {"Volumen", game_settings::volumeLabel(state.settings.volume), kThemeAccentGreen},
        {"Dificultad", game_settings::difficultyLabel(state.settings.difficulty), kThemeAccent},
        {"Velocidad", game_settings::simulationSpeedLabel(state.settings.simulationSpeed), kThemeWarning},
        {"Modo", game_settings::simulationModeLabel(state.settings.simulationMode), kThemeAccentBlue},
        {"Estado", "Listo", kThemeAccentGreen}
    };

    model.summary.content =
        "Cabina de ajustes\r\n\r\n"
        "[Volumen]\r\n"
        "Se mantiene como control de frontend aunque el juego aun no tenga audio real.\r\n\r\n"
        "[Dificultad]\r\n"
        "Afecta el arranque de nuevas carreras y el contexto del proyecto.\r\n\r\n"
        "[Velocidad de simulacion]\r\n"
        "Define el ritmo percibido del frontend y deja lista la estructura para futuras animaciones o pausas.\r\n\r\n"
        "[Modo de simulacion]\r\n"
        "Controla si el juego prioriza una lectura rapida o una salida mas detallada.";

    std::ostringstream detail;
    detail << "Impacto sobre la experiencia\r\n\r\n";
    detail << "Volumen: " << game_settings::volumeLabel(state.settings.volume) << "\r\n";
    detail << "Dificultad: " << game_settings::difficultyLabel(state.settings.difficulty) << "\r\n";
    detail << game_settings::difficultyDescription(state.settings.difficulty) << "\r\n\r\n";
    detail << "Velocidad: " << game_settings::simulationSpeedLabel(state.settings.simulationSpeed) << "\r\n";
    detail << game_settings::simulationSpeedDescription(state.settings.simulationSpeed) << "\r\n\r\n";
    detail << "Simulacion: " << game_settings::simulationModeLabel(state.settings.simulationMode) << "\r\n";
    detail << game_settings::simulationModeDescription(state.settings.simulationMode) << "\r\n\r\n";
    detail << "Usa los botones del centro para ir ciclando cada valor y vuelve limpio al menu principal.";
    model.detail.content = detail.str();

    model.feed.lines = {
        std::string("Volumen actual: ") + game_settings::volumeLabel(state.settings.volume),
        std::string("Dificultad actual: ") + game_settings::difficultyLabel(state.settings.difficulty),
        std::string("Velocidad actual: ") + game_settings::simulationSpeedLabel(state.settings.simulationSpeed),
        std::string("Modo actual: ") + game_settings::simulationModeLabel(state.settings.simulationMode),
        "Volver te devuelve al menu principal sin perder cambios."
    };
    return model;
}

}  // namespace gui_win32

#endif
