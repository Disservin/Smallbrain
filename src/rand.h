#pragma once
#include <random>

namespace rand_gen {
static std::random_device rd;
static std::mt19937 generator(rd());
}  // namespace rand