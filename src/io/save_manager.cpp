#include "io/save_manager.h"

#include "io/save_serialization.h"
#include "utils/utils.h"

#include <algorithm>
#include <cstdio>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

namespace save_manager {

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
    if (career.saveFile.rfind("saves/", 0) == 0 || career.saveFile.rfind("saves\\", 0) == 0) {
        ensureDirectory("saves");
    }

    const string tempPath = career.saveFile + ".tmp";
    const string backupPath = career.saveFile + ".bak";
    const bool backupReady = copyExistingFile(career.saveFile, backupPath);

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

    if (!replaceFileAtomically(tempPath, career.saveFile)) {
        if (overwriteFromTempFile(tempPath, career.saveFile)) {
            std::remove(tempPath.c_str());
            return true;
        }
        std::remove(tempPath.c_str());
        if (backupReady && pathExists(backupPath)) {
            overwriteFromTempFile(backupPath, career.saveFile);
        }
        return false;
    }
    return true;
}

bool loadCareer(Career& career) {
    string resolvedSave = career.saveFile;
    if (!pathExists(resolvedSave) && resolvedSave == "saves/career_save.txt" && pathExists("career_save.txt")) {
        resolvedSave = "career_save.txt";
    }

    ifstream file(resolvedSave, ios::binary);
    if (!file.is_open()) return false;
    return save_serialization::deserializeCareer(file, career);
}

}  // namespace save_manager

bool Career::saveCareer() {
    return save_manager::saveCareer(*this);
}

bool Career::loadCareer() {
    return save_manager::loadCareer(*this);
}
