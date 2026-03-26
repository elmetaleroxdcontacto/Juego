#include "gui/gui_audio.h"

#ifdef _WIN32

#include <mmsystem.h>

#include <sstream>
#include <vector>

namespace gui_win32 {

namespace {

constexpr wchar_t kMenuMusicAlias[] = L"cf_menu_music";
constexpr wchar_t kMenuMusicFileName[] = L"Los Miserables - El Crack  Video Oficial (HD Remastered).mp3";

bool fileExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring canonicalPath(const std::wstring& path) {
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetFullPathNameW(path.c_str(), MAX_PATH, buffer, nullptr);
    if (length == 0 || length >= MAX_PATH) return path;
    return std::wstring(buffer, buffer + length);
}

std::wstring directoryOf(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return std::wstring();
    return path.substr(0, slash);
}

std::wstring joinPath(const std::wstring& base, const std::wstring& child) {
    if (base.empty()) return child;
    if (base.back() == L'\\' || base.back() == L'/') return base + child;
    return base + L"\\" + child;
}

std::wstring executableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return std::wstring();
    return directoryOf(std::wstring(buffer, buffer + length));
}

std::wstring currentDirectoryPath() {
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetCurrentDirectoryW(MAX_PATH, buffer);
    if (length == 0 || length >= MAX_PATH) return std::wstring();
    return std::wstring(buffer, buffer + length);
}

std::wstring resolveMenuMusicPath() {
    const std::wstring exeDir = executableDirectory();
    const std::wstring cwd = currentDirectoryPath();
    const std::vector<std::wstring> candidates = {
        joinPath(cwd, kMenuMusicFileName),
        joinPath(exeDir, kMenuMusicFileName),
        canonicalPath(joinPath(exeDir, std::wstring(L"..\\") + kMenuMusicFileName)),
        canonicalPath(joinPath(exeDir, std::wstring(L"..\\..\\") + kMenuMusicFileName))
    };

    for (const auto& candidate : candidates) {
        if (candidate.empty()) continue;
        const std::wstring normalized = canonicalPath(candidate);
        if (fileExists(normalized)) return normalized;
    }
    return std::wstring();
}

bool sendMci(const std::wstring& command) {
    return mciSendStringW(command.c_str(), nullptr, 0, nullptr) == 0;
}

void closeMenuMusicDevice(AppState& state) {
    if (state.menuMusicOpened) {
        sendMci(L"close " + std::wstring(kMenuMusicAlias));
    }
    state.menuMusicOpened = false;
    state.menuMusicPlaying = false;
}

int volumeToMciScale(const AppState& state) {
    const int clamped = game_settings::clampVolume(state.settings.volume);
    return (clamped * 1000) / 100;
}

void applyMenuMusicVolume(AppState& state) {
    if (!state.menuMusicOpened) return;
    std::wostringstream command;
    command << L"setaudio " << kMenuMusicAlias << L" volume to " << volumeToMciScale(state);
    sendMci(command.str());
}

void ensureMenuMusicOpened(AppState& state) {
    if (state.menuMusicOpened) return;
    if (state.menuMusicPath.empty()) {
        state.menuMusicPath = resolveMenuMusicPath();
    }
    if (state.menuMusicPath.empty()) {
        state.menuMusicMissingReported = true;
        return;
    }

    std::wstring openCommand = L"open \"" + state.menuMusicPath + L"\" type mpegvideo alias " + std::wstring(kMenuMusicAlias);
    if (!sendMci(openCommand)) {
        state.menuMusicMissingReported = true;
        state.menuMusicPath.clear();
        return;
    }

    state.menuMusicOpened = true;
    applyMenuMusicVolume(state);
}

}  // namespace

void syncMenuMusicForPage(AppState& state) {
    const bool shouldPlay = state.currentPage == GuiPage::MainMenu;
    if (!shouldPlay) {
        if (state.menuMusicOpened && state.menuMusicPlaying) {
            sendMci(L"stop " + std::wstring(kMenuMusicAlias));
            sendMci(L"seek " + std::wstring(kMenuMusicAlias) + L" to start");
            state.menuMusicPlaying = false;
        }
        return;
    }

    ensureMenuMusicOpened(state);
    if (!state.menuMusicOpened || state.menuMusicPlaying) return;

    applyMenuMusicVolume(state);
    if (sendMci(L"play " + std::wstring(kMenuMusicAlias) + L" repeat")) {
        state.menuMusicPlaying = true;
    }
}

void refreshMenuMusicVolume(AppState& state) {
    if (!state.menuMusicOpened) return;
    applyMenuMusicVolume(state);
}

void shutdownMenuMusic(AppState& state) {
    closeMenuMusicDevice(state);
}

}  // namespace gui_win32

#endif
