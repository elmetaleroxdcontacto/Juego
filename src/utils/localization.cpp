#include "utils/localization.h"

Localization& Localization::getInstance() {
    static Localization instance;
    return instance;
}

Localization::Localization() : currentLang(Language::Spanish) {
    loadTexts();
}

void Localization::setLanguage(Language lang) {
    currentLang = lang;
}

std::string Localization::getText(const std::string& key) const {
    auto it = texts.find(key);
    if (it != texts.end()) {
        auto langIt = it->second.find(currentLang);
        if (langIt != it->second.end()) {
            return langIt->second;
        }
    }
    return key; // Fallback to key
}

void Localization::loadTexts() {
    // Example texts
    texts["menu_continue"] = {
        {Language::Spanish, "Continuar"},
        {Language::English, "Continue"},
        {Language::Portuguese, "Continuar"},
        {Language::French, "Continuer"}
    };
    texts["menu_new_game"] = {
        {Language::Spanish, "Jugar"},
        {Language::English, "New Game"},
        {Language::Portuguese, "Jogar"},
        {Language::French, "Jouer"}
    };
    // Add more as needed
}