#include "gui/gui_view_builders.h"

#ifdef _WIN32

#include "engine/game_settings.h"
#include "utils/utils.h"

#include <sstream>

namespace gui_win32 {

GuiPageModel buildMainMenuModel(AppState& state) {
    GuiPageModel model;
    model.title = game_settings::gameTitle();
    model.breadcrumb = "Inicio > Menu principal";
    model.infoLine = "Portada principal del manager. Continuar, cargar, creditos y configuraciones viven en el mismo frontend real.";
    model.summary.title = "FrontMenuOverview";
    model.detail.title = "FrontMenuProfile";
    model.feed.title = "FrontMenuRoadmap";
    model.metrics = {
        {"Continuar", state.career.myTeam ? "Sesion activa" : (pathExists(state.career.saveFile) || pathExists("career_save.txt") ? "Guardado listo" : "Sin guardado"), kThemeAccentBlue},
        {"Dificultad", game_settings::difficultyLabel(state.settings.difficulty), kThemeAccent},
        {"Velocidad", game_settings::simulationSpeedLabel(state.settings.simulationSpeed), kThemeWarning},
        {"Modo", game_settings::simulationModeLabel(state.settings.simulationMode), kThemeAccentBlue},
        {"Audio", game_settings::menuMusicModeLabel(state.settings.menuMusicMode), kThemeAccentGreen}
    };

    model.summary.content =
        "Panel de acceso\r\n"
        "- Continuar retoma la sesion activa o intenta cargar el ultimo guardado.\r\n"
        "- Jugar abre el flujo real del proyecto: dashboard, club, carrera, mercado, tacticas y noticias.\r\n"
        "- Cargar guardado fuerza la lectura del ultimo save disponible.\r\n"
        "- Configuraciones abre la cabina persistente de frontend.\r\n"
        "- Creditos y Salir completan la portada como una base real, no decorativa.";

    std::ostringstream detail;
    detail << "Volumen: " << game_settings::volumeLabel(state.settings.volume) << "\r\n";
    detail << "Dificultad: " << game_settings::difficultyLabel(state.settings.difficulty) << "\r\n";
    detail << game_settings::difficultyDescription(state.settings.difficulty) << "\r\n\r\n";
    detail << "Velocidad de simulacion: " << game_settings::simulationSpeedLabel(state.settings.simulationSpeed) << "\r\n";
    detail << game_settings::simulationSpeedDescription(state.settings.simulationSpeed) << "\r\n\r\n";
    detail << "Modo de simulacion: " << game_settings::simulationModeLabel(state.settings.simulationMode) << "\r\n";
    detail << game_settings::simulationModeDescription(state.settings.simulationMode) << "\r\n\r\n";
    detail << "Idioma: " << game_settings::languageLabel(state.settings.language) << "\r\n";
    detail << "Texto: " << game_settings::textSpeedLabel(state.settings.textSpeed) << "\r\n";
    detail << "Visual: " << game_settings::visualProfileLabel(state.settings.visualProfile) << "\r\n";
    detail << "Musica: " << game_settings::menuMusicModeLabel(state.settings.menuMusicMode) << " | "
           << game_settings::menuAudioFadeLabel(state.settings.menuAudioFade) << "\r\n\r\n";
    detail << "Perfil persistente listo para reabrir el centro del club sin perder contexto.";
    model.detail.content = detail.str();

    model.feed.lines = {
        "Estado actual: frontend listo para una experiencia tipo manager game.",
        "Navegacion GUI: Tab, Enter, flechas, numeros, F11 y Esc desde ajustes o creditos.",
        "Continuar y Cargar ya usan el flujo real del proyecto en vez de una ruta separada.",
        "La portada mantiene base lista para perfil, video, idiomas y resolucion."
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
        {"Visual", game_settings::visualProfileLabel(state.settings.visualProfile), kThemeAccentBlue},
        {"Musica", game_settings::menuMusicModeLabel(state.settings.menuMusicMode), kThemeAccent}
    };

    model.summary.content =
        "[Volumen]\r\n"
        "Se mantiene como control de frontend aunque el juego aun no tenga audio real.\r\n\r\n"
        "[Dificultad]\r\n"
        "Afecta el arranque de nuevas carreras y el contexto del proyecto.\r\n\r\n"
        "[Velocidad de simulacion]\r\n"
        "Define el ritmo percibido del frontend y deja lista la estructura para futuras animaciones o pausas.\r\n\r\n"
        "[Modo de simulacion]\r\n"
        "Controla si el juego prioriza una lectura rapida o una salida mas detallada.\r\n\r\n"
        "[Idioma, texto y visual]\r\n"
        "Preparan accesibilidad, densidad y tono visual del frontend.\r\n\r\n"
        "[Musica del frontend]\r\n"
        "Permite silenciar el tema o extenderlo a portada, ajustes y creditos.";

    std::ostringstream detail;
    detail << "Volumen: " << game_settings::volumeLabel(state.settings.volume) << "\r\n";
    detail << "Dificultad: " << game_settings::difficultyLabel(state.settings.difficulty) << "\r\n";
    detail << game_settings::difficultyDescription(state.settings.difficulty) << "\r\n\r\n";
    detail << "Velocidad: " << game_settings::simulationSpeedLabel(state.settings.simulationSpeed) << "\r\n";
    detail << game_settings::simulationSpeedDescription(state.settings.simulationSpeed) << "\r\n\r\n";
    detail << "Simulacion: " << game_settings::simulationModeLabel(state.settings.simulationMode) << "\r\n";
    detail << game_settings::simulationModeDescription(state.settings.simulationMode) << "\r\n\r\n";
    detail << "Idioma: " << game_settings::languageLabel(state.settings.language) << "\r\n";
    detail << game_settings::languageDescription(state.settings.language) << "\r\n\r\n";
    detail << "Texto: " << game_settings::textSpeedLabel(state.settings.textSpeed) << "\r\n";
    detail << game_settings::textSpeedDescription(state.settings.textSpeed) << "\r\n\r\n";
    detail << "Visual: " << game_settings::visualProfileLabel(state.settings.visualProfile) << "\r\n";
    detail << game_settings::visualProfileDescription(state.settings.visualProfile) << "\r\n\r\n";
    detail << "Musica: " << game_settings::menuMusicModeLabel(state.settings.menuMusicMode) << "\r\n";
    detail << game_settings::menuMusicModeDescription(state.settings.menuMusicMode) << "\r\n";
    detail << game_settings::menuAudioFadeDescription(state.settings.menuAudioFade) << "\r\n\r\n";
    detail << "Los cambios se guardan automaticamente en disco y vuelven al abrir el juego.";
    model.detail.content = detail.str();

    model.feed.lines = {
        std::string("Volumen actual: ") + game_settings::volumeLabel(state.settings.volume),
        std::string("Dificultad actual: ") + game_settings::difficultyLabel(state.settings.difficulty),
        std::string("Velocidad actual: ") + game_settings::simulationSpeedLabel(state.settings.simulationSpeed),
        std::string("Modo actual: ") + game_settings::simulationModeLabel(state.settings.simulationMode),
        std::string("Idioma / texto: ") + game_settings::languageLabel(state.settings.language) + " / " + game_settings::textSpeedLabel(state.settings.textSpeed),
        std::string("Visual / musica: ") + game_settings::visualProfileLabel(state.settings.visualProfile) + " / " + game_settings::menuMusicModeLabel(state.settings.menuMusicMode)
    };
    return model;
}

GuiPageModel buildCreditsPageModel(AppState& state) {
    GuiPageModel model;
    model.title = "Creditos";
    model.breadcrumb = "Inicio > Creditos";
    model.infoLine = "Identidad del proyecto, base tecnica y direccion actual de Chilean Footballito.";
    model.summary.title = "FrontMenuOverview";
    model.detail.title = "FrontMenuProfile";
    model.feed.title = "FrontMenuRoadmap";
    model.metrics = {
        {"Motor", "C++17", kThemeAccentBlue},
        {"Frontend", "Win32 + CLI", kThemeAccent},
        {"Audio", game_settings::menuThemeLabel(state.settings), kThemeAccentGreen},
        {"Visual", game_settings::visualProfileLabel(state.settings.visualProfile), kThemeWarning},
        {"Estado", "Escalable", kThemeAccentBlue}
    };

    model.summary.content =
        "Chilean Footballito\r\n"
        "Simulador de gestion futbolistica construido en C++ con una arquitectura modular que une carrera, staff, scouting, mercado, motor de partido y frontend compartido.\r\n\r\n"
        "La GUI Win32 y la CLI consumen la misma capa de servicios, settings y persistencia.";

    std::ostringstream detail;
    detail << "Tecnologia\r\n";
    detail << "- C++17\r\n";
    detail << "- GUI Win32 nativa\r\n";
    detail << "- CLI compartida para validacion y depuracion\r\n";
    detail << "- Persistencia propia para carrera y configuracion\r\n\r\n";
    detail << "Tema del menu\r\n";
    detail << "- Track activo: " << game_settings::menuThemeLabel(state.settings) << "\r\n";
    detail << "- Alcance: " << game_settings::menuMusicModeLabel(state.settings.menuMusicMode) << "\r\n";
    detail << "- Transicion: " << game_settings::menuAudioFadeLabel(state.settings.menuAudioFade) << "\r\n\r\n";
    detail << "Objetivo\r\n";
    detail << "Seguir acercando el proyecto a una experiencia profunda tipo manager game.";
    model.detail.content = detail.str();

    model.feed.lines = {
        "Arquitectura preparada para continuar, cargar, creditos ampliados y opciones de video.",
        "Settings persistentes unificados entre consola, GUI, audio y timing del frontend.",
        "Audio y assets del menu ya viven en assets/audio con manifiesto documentado.",
        "Volver te devuelve limpio a la portada principal."
    };
    return model;
}

}  // namespace gui_win32

#endif
