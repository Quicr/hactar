#pragma once

#include <cstdint>

void FibMain();
extern "C" int32_t FibInt32(uint32_t);
extern "C" int64_t FibInt64(uint32_t);
extern "C" float FibFloat32(uint32_t);
extern "C" double FibFloat64(uint32_t);