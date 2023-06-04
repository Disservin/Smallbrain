#pragma once
#include <random>

namespace random {
static std::random_device rd;
static std::mt19937 generator(rd());
}  // namespace random