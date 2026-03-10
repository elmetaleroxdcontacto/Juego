#pragma once

#include "engine/models.h"

#include <iosfwd>

namespace save_serialization {

int currentCareerSaveVersion();
bool serializeCareer(std::ostream& file, const Career& career);
bool deserializeCareer(std::istream& file, Career& career);

}  // namespace save_serialization
