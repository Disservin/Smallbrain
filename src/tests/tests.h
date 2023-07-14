#pragma once

#include <functional>
#include <string>

#include "../board.h"
#include "../movegen.h"

// #define unit_test(func, return_value, ...)
//     std::cout << "Running " << __func__ << std::endl;
//     assert(func(__VA_OPT__(__VA_ARGS__)) == return_value);

// void unit_test(auto &&func, auto &&return_value, auto &&...args)
// {
//     std::cout << "Input:" << std::endl;
//     ((std::cout << ' ' << args << std::endl), ...);
//     assert(std::invoke(std::forward<decltype(func)>(func), std::forward<decltype(args)>(args)...)
//     ==
//            std::forward<decltype(return_value)>(return_value));
// }

#define expect(got, expected, input)                  \
    if (got != expected) {                            \
        std::cout << "Input: " << input << std::endl; \
        assert(got == expected);                      \
    }
// assert to catch file and line and function info

namespace tests {

bool testall();

}  // namespace tests