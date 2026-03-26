#include "gui/gui_view_builders.h"

#ifdef _WIN32

#include "engine/game_settings.h"

#include <sstream>

namespace gui_win32 {

GuiPageModel buildMainMenuModel(AppState& state) {
    GuiPageModel model;
    model.title = game_settings::gameTitle();
    model.breadcrumb = "Inicio > Menu principal";
    model.infoLine = "Arranque principal del juego. Entra a Jugar para abrir el flujo normal o revisa Configuraciones antes de empezar.";
    model.summary.title = "FrontMenuWelcome";
    model.detail.title = "FrontMenuConfig";
    model.feed.title = "FrontMenuActions";
    model.metrics = {
        {"Titulo", game_settings::gameTitle(), kThemeAccentBlue},
        {"Volumen", game_settings::volumeLabel(state.settings.volume), kThemeAccentGreen},
        {"Dificultad", game_settings::difficultyLabel(state.settings.difficulty), kThemeAccent},
        {"Simulacion", game_settings::simulationModeLabel(state.settings.simulationMode), kThemeAccentBlue},
        {"Estado", "Menu principal", kThemeWarning}
    };

    model.summary.content =
        "Bienvenido a Chilean Footballito\r\n\r\n"
        "Desde aqui puedes entrar al flujo principal del juego sin saltarte ningun paso del frontend.\r\n\r\n"
        "[Jugar]\r\n"
        "- abre el centro del club y el flujo normal actual\r\n"
        "- mantiene intactas las pantallas de carrera, fichajes, tacticas y noticias\r\n\r\n"
        "[Configuraciones]\r\n"
        "- ajusta volumen, dificultad y modo de simulacion\r\n"
        "- deja el proyecto listo antes de empezar una carrera";

    std::ostringstream detail;
    detail << "Perfil de juego actual\r\n\r\n";
    detail << "Volumen: " << game_settings::volumeLabel(state.settings.volume) << "\r\n";
    detail << "Dificultad: " << game_settings::difficultyLabel(state.settings.difficulty) << "\r\n";
    detail << game_settings::difficultyDescription(state.settings.difficulty) << "\r\n\r\n";
    detail << "Modo de simulacion: " << game_settings::simulationModeLabel(state.settings.simulationMode) << "\r\n";
    detail << game_settings::simulationModeDescription(state.settings.simulationMode) << "\r\n\r\n";
    detail << "Pulsa Jugar para abrir el flujo normal del juego.";
    model.detail.content = detail.str();

    model.feed.lines = {
        "Jugar -> entra al dashboard y al flujo normal del juego.",
        "Configuraciones -> abre el menu de ajustes iniciales.",
        "F11 sigue disponible para alternar modos de pantalla en cualquier momento."
    };
    return model;
}

GuiPageModel buildSettingsPageModel(AppState& state) {
    GuiPageModel model;
    model.title = "Configuraciones";
    model.breadcrumb = "Inicio > Configuraciones";
    model.infoLine = "Ajustes basicos listos para crecer: volumen, dificultad y modo de simulacion.";
    model.summary.title = "SettingsOverview";
    model.detail.title = "SettingsImpact";
    model.feed.title = "SettingsChecklist";
    model.metrics = {
        {"Volumen", game_settings::volumeLabel(state.settings.volume), kThemeAccentGreen},
        {"Dificultad", game_settings::difficultyLabel(state.settings.difficulty), kThemeAccent},
        {"Simulacion", game_settings::simulationModeLabel(state.settings.simulationMode), kThemeAccentBlue},
        {"Audio", "Simulado", kThemeWarning},
        {"Estado", "Listo", kThemeAccentGreen}
    };

    model.summary.content =
        "Ajustes disponibles\r\n\r\n"
        "[Volumen]\r\n"
        "Se mantiene como control de frontend aunque el juego aun no tenga audio real.\r\n\r\n"
        "[Dificultad]\r\n"
        "Afecta el arranque de nuevas carreras y el contexto del proyecto.\r\n\r\n"
        "[Modo de simulacion]\r\n"
        "Controla si el juego prioriza una lectura rapida o una salida mas detallada.";

    std::ostringstream detail;
    detail << "Impacto actual\r\n\r\n";
    detail << "Volumen: " << game_settings::volumeLabel(state.settings.volume) << "\r\n";
    detail << "Dificultad: " << game_settings::difficultyLabel(state.settings.difficulty) << "\r\n";
    detail << game_settings::difficultyDescription(state.settings.difficulty) << "\r\n\r\n";
    detail << "Simulacion: " << game_settings::simulationModeLabel(state.settings.simulationMode) << "\r\n";
    detail << game_settings::simulationModeDescription(state.settings.simulationMode) << "\r\n\r\n";
    detail << "Usa los botones del centro para ir ciclando cada valor.";
    model.detail.content = detail.str();

    model.feed.lines = {
        std::string("Volumen actual: ") + game_settings::volumeLabel(state.settings.volume),
        std::string("Dificultad actual: ") + game_settings::difficultyLabel(state.settings.difficulty),
        std::string("Simulacion actual: ") + game_settings::simulationModeLabel(state.settings.simulationMode),
        "Volver te devuelve al menu principal sin perder los cambios."
    };
    return model;
}

}  // namespace gui_win32

#endif
