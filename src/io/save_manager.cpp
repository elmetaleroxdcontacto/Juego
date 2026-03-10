#include "io/save_manager.h"

#include "io/save_serialization.h"
#include "utils/utils.h"

#include <fstream>

using namespace std;

namespace save_manager {

bool saveCareer(Career& career) {
    if (career.saveFile.rfind("saves/", 0) == 0 || career.saveFile.rfind("saves\\", 0) == 0) {
        ensureDirectory("saves");
    }
    ofstream file(career.saveFile);
    if (!file.is_open()) return false;
    return save_serialization::serializeCareer(file, career);
}

bool loadCareer(Career& career) {
    string resolvedSave = career.saveFile;
    if (!pathExists(resolvedSave) && resolvedSave == "saves/career_save.txt" && pathExists("career_save.txt")) {
        resolvedSave = "career_save.txt";
    }

    ifstream file(resolvedSave);
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
