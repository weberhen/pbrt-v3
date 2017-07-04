#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_CORE_MAKESCENE_H
#define PBRT_CORE_MAKESCENE_H

// core/parser.h*
#include "pbrt.h"
#include "api.h"
#include "paramset.h"
#include "tools/json.hpp"
#include <fstream>
#include <stdlib.h>     /* atof */

using json = nlohmann::json;

namespace pbrt {

void MakeScene(std::string filename);

}  // namespace pbrt

#endif  // PBRT_CORE_MAKESCENE_H
