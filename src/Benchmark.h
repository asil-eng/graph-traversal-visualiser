#pragma once

#include "Algorithms.h"
#include "Grid.h"
#include <cstdint>
#include <vector>

struct BenchmarkEntry {
    Algorithm algo;
    int nodesVisited = 0;
    int pathCost = 0;  // sum of step costs along the path (weights count as 5)
    bool found = false;
};

// Runs all four algorithms on the current layout and ranks them with a
// hand-written merge sort (path found first, then fewest nodes visited, then
// cheapest path). Results are cached per layout in a hand-written hash table
// (division-method hashing, linear probing) so re-benchmarking the same maze
// is an O(1) lookup instead of four full searches.
class Benchmark {
public:
    std::vector<BenchmarkEntry> run(const Grid& grid, bool& fromCache);

private:
    static constexpr int TABLE_SIZE = 61;  // prime, as the division method prefers

    struct Slot {
        bool used = false;
        std::uint64_t key = 0;
        GridLayout layout;  // stored to rule out fingerprint collisions
        std::vector<BenchmarkEntry> ranking;
    };

    static std::uint64_t fingerprint(const GridLayout& layout);
    int probe(std::uint64_t key, const GridLayout& layout) const;

    Slot m_table[TABLE_SIZE];
};
