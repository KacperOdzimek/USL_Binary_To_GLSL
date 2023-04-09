#pragma once
#include "StandardVersion.h"

#include <cstdint>
#include <vector>

std::vector<uint8_t> LoadMathExpression(uint8_t*& iterator, const StandardVersion* version, Temp* temp);