#include "io/save_manager.h"

#include "io/save_serialization.h"
#include "utils/utils.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

namespace save_manager {

namespace {

string normalizeSavePath(const string& path) {
    return trim(path.empty() ? string("saves/career_save.txt") : path);
}

string resolveSavePath(const string& path) {
    const string normalized = normalizeSavePath(path);
    return resolveProjectPath(normalized);
}

string parentDirectory(const string& path) {
    const string filename = pathFilename(path);
    if (filename == path) return string();
    return path.substr(0, path.size() - filename.size());
}

}  // namespace

#ifdef _WIN32
wstring utf8ToWidePath(const string& text) {
    if (text.empty()) return wstring();
    int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 0) return wstring(text.begin(), text.end());
    wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wide[0], length);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    replace(wide.begin(), wide.end(), L'/', L'\\');
    return wide;
}
#endif

bool replaceFileAtomically(const string& sourcePath, const string& targetPath) {
#ifdef _WIN32
    const wstring wideSource = utf8ToWidePath(sourcePath);
    const wstring wideTarget = utf8ToWidePath(targetPath);
    if (wideSource.empty() || wideTarget.empty()) return false;

    if (MoveFileExW(wideSource.c_str(),
                    wideTarget.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) != 0) {
        return true;
    }

    const DWORD targetAttributes = GetFileAttributesW(wideTarget.c_str());
    if (targetAttributes != INVALID_FILE_ATTRIBUTES) {
        if (ReplaceFileW(wideTarget.c_str(),
                         wideSource.c_str(),
                         nullptr,
                         REPLACEFILE_IGNORE_MERGE_ERRORS,
                         nullptr,
                         nullptr) != 0) {
            return true;
        }

        if ((targetAttributes & FILE_ATTRIBUTE_READONLY) != 0) {
            SetFileAttributesW(wideTarget.c_str(), targetAttributes & ~FILE_ATTRIBUTE_READONLY);
        }
        DeleteFileW(wideTarget.c_str());
        if (MoveFileExW(wideSource.c_str(), wideTarget.c_str(), MOVEFILE_WRITE_THROUGH) != 0) {
            return true;
        }
    }

    std::remove(targetPath.c_str());
    return std::rename(sourcePath.c_str(), targetPath.c_str()) == 0;
#else
    std::remove(targetPath.c_str());
    return std::rename(sourcePath.c_str(), targetPath.c_str()) == 0;
#endif
}

bool overwriteFromTempFile(const string& sourcePath, const string& targetPath) {
    ifstream source(sourcePath, ios::binary);
    if (!source.is_open()) return false;

    ofstream target(targetPath, ios::binary | ios::trunc);
    if (!target.is_open()) return false;

    target << source.rdbuf();
    return (source.good() || source.eof()) ? target.good() : false;
}

bool copyExistingFile(const string& sourcePath, const string& targetPath) {
    if (!pathExists(sourcePath)) return true;
#ifdef _WIN32
    const wstring wideSource = utf8ToWidePath(sourcePath);
    const wstring wideTarget = utf8ToWidePath(targetPath);
    if (!wideSource.empty() && !wideTarget.empty()) {
        if (CopyFileW(wideSource.c_str(), wideTarget.c_str(), FALSE) != 0) return true;
    }
#endif
    return overwriteFromTempFile(sourcePath, targetPath);
}

bool saveCareer(Career& career) {
    career.saveFile = normalizeSavePath(career.saveFile);
    const string savePath = resolveSavePath(career.saveFile);
    const string saveFolder = parentDirectory(savePath);
    if (!saveFolder.empty() && !ensureDirectory(saveFolder)) return false;

    const string tempPath = savePath + ".tmp";
    const string backupPath = savePath + ".bak";
    const bool backupReady = copyExistingFile(savePath, backupPath);

    ofstream file(tempPath, ios::binary | ios::trunc);
    if (!file.is_open()) return false;

    const bool serialized = save_serialization::serializeCareer(file, career);
    file.flush();
    const bool fileOk = file.good();
    file.close();

    if (!serialized || !fileOk) {
        std::remove(tempPath.c_str());
        return false;
    }

    if (!replaceFileAtomically(tempPath, savePath)) {
        if (overwriteFromTempFile(tempPath, savePath)) {
            std::remove(tempPath.c_str());
            return true;
        }
        std::remove(tempPath.c_str());
        if (backupReady && pathExists(backupPath)) {
            overwriteFromTempFile(backupPath, savePath);
        }
        return false;
    }
    return true;
}

bool loadCareer(Career& career) {
    const string requestedSave = normalizeSavePath(career.saveFile);
    string resolvedSave = resolveSavePath(requestedSave);
    if (!pathExists(resolvedSave) && requestedSave == "saves/career_save.txt") {
        resolvedSave = resolveProjectPath("career_save.txt");
    }

    ifstream file(resolvedSave, ios::binary);
    if (!file.is_open()) return false;

    Career loadedCareer;
    loadedCareer.saveFile = requestedSave;
    if (!save_serialization::deserializeCareer(file, loadedCareer)) return false;

    const string controlledTeam = loadedCareer.myTeam ? loadedCareer.myTeam->name : "";
    career = std::move(loadedCareer);
    career.saveFile = requestedSave;
    if (career.activeDivision.empty() && !career.allTeams.empty()) {
        career.activeDivision = career.allTeams.front().division;
    }
    if (!career.activeDivision.empty()) {
        career.setActiveDivision(career.activeDivision);
    }
    career.myTeam = controlledTeam.empty() ? nullptr : career.findTeamByName(controlledTeam);
    return true;
}

}  // namespace save_manager

bool Career::saveCareer() {
    return save_manager::saveCareer(*this);
}

bool Career::loadCareer() {
    return save_manager::loadCareer(*this);
}

