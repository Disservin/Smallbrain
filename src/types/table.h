#pragma once

#include <array>

/// @brief Table template class for creating N-dimensional arrays.
/// @tparam T
/// @tparam N
/// @tparam ...Dims
template <typename T, size_t N, size_t... Dims>
struct Table {
    std::array<Table<T, Dims...>, N> data;
    Table() { data.fill({}); }
    Table<T, Dims...> &operator[](size_t index) { return data[index]; }
    const Table<T, Dims...> &operator[](size_t index) const { return data[index]; }

    void reset() { data.fill({}); }
};

template <typename T, size_t N>
struct Table<T, N> {
    std::array<T, N> data;
    Table() { data.fill({}); }
    T &operator[](size_t index) { return data[index]; }
    const T &operator[](size_t index) const { return data[index]; }

    void reset() { data.fill({}); }
};