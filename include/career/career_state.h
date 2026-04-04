#pragma once

#include <string>
#include <vector>
#include <deque>

struct DivisionInfo {
    std::string id;
    std::string folder;
    std::string display;
};

class CareerState {
public:
    int currentSeason;
    int currentWeek;
    std::string activeDivision;
    std::string saveFile;
    std::vector<DivisionInfo> divisions;
    bool initialized;

    CareerState();
    bool usesSegundaFormat() const;
    bool usesTerceraBFormat() const;
    bool usesGroupFormat() const;
    void setActiveDivision(const std::string& id);
};