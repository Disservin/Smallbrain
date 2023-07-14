#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

#include "search.h"

// A wrapper class to start the search
class SearchInstance {
public:
    SearchInstance() { search = std::make_unique<Search>(); }

    SearchInstance(const SearchInstance &other) {
        search = std::make_unique<Search>(*other.search);
    }

    SearchInstance &operator=(const SearchInstance &other) {
        // protect against invalid self-assignment
        if (this != &other) {
            search = std::make_unique<Search>(*other.search);
        }
        // by convention, always return *this
        return *this;
    }

    ~SearchInstance() = default;

    void start() const;

    std::unique_ptr<Search> search;
};

// Holds all currently running threads and their data
class ThreadPool {
public:
    [[nodiscard]] U64 getNodes() const;

    [[nodiscard]] U64 getTbHits() const;

    void start(const Board &board, const Limits &limit, const Movelist &searchmoves,
               int worker_count, bool use_tb);

    void kill();

    std::atomic_bool stop;

private:
    std::vector<SearchInstance> pool_;
    std::vector<std::thread> running_threads_;
};