#pragma once

#include <string>
#include <unordered_map>

class Localization {
public:
    enum class Language { Spanish, English, Portuguese, French };

    static Localization& getInstance();
    void setLanguage(Language lang);
    std::string getText(const std::string& key) const;

private:
    Localization();
    Language currentLang;
    std::unordered_map<std::string, std::unordered_map<Language, std::string>> texts;

    void loadTexts();
};