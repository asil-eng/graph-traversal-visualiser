#include "Benchmark.h"

namespace {

// True if `a` should rank above `b`.
bool ranksBefore(const BenchmarkEntry& a, const BenchmarkEntry& b) {
    if (a.found != b.found) return a.found;
    if (a.nodesVisited != b.nodesVisited) return a.nodesVisited < b.nodesVisited;
    return a.pathCost < b.pathCost;
}

void merge(std::vector<BenchmarkEntry>& items, std::vector<BenchmarkEntry>& temp,
           int lo, int mid, int hi) {
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        // Take from the right half only if strictly better, which keeps the sort stable.
        if (ranksBefore(items[j], items[i])) {
            temp[k++] = items[j++];
        } else {
            temp[k++] = items[i++];
        }
    }
    while (i < mid) temp[k++] = items[i++];
    while (j < hi) temp[k++] = items[j++];
    for (int x = lo; x < hi; ++x) items[x] = temp[x];
}

// Sorts items[lo, hi).
void mergeSort(std::vector<BenchmarkEntry>& items, std::vector<BenchmarkEntry>& temp, int lo, int hi) {
    if (hi - lo < 2) return;
    int mid = lo + (hi - lo) / 2;
    mergeSort(items, temp, lo, mid);
    mergeSort(items, temp, mid, hi);
    merge(items, temp, lo, mid, hi);
}

int pathCost(const Grid& grid, const std::vector<Coord>& path) {
    int total = 0;
    for (std::size_t i = 1; i < path.size(); ++i) {
        total += grid.cost(path[i].row, path[i].col);
    }
    return total;
}

}  // namespace

std::uint64_t Benchmark::fingerprint(const GridLayout& layout) {
    // FNV-1a over every cell plus the start/end positions.
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&h](std::uint64_t v) {
        h ^= v;
        h *= 1099511628211ULL;
    };
    for (const auto& row : layout.cells) {
        for (CellType t : row) mix(static_cast<std::uint64_t>(t));
    }
    mix(static_cast<std::uint64_t>(layout.start.row));
    mix(static_cast<std::uint64_t>(layout.start.col));
    mix(static_cast<std::uint64_t>(layout.end.row));
    mix(static_cast<std::uint64_t>(layout.end.col));
    return h;
}

// Returns the slot holding `layout`, or the first empty slot on its probe
// sequence, or -1 if the table is full and the layout is absent.
int Benchmark::probe(std::uint64_t key, const GridLayout& layout) const {
    int home = static_cast<int>(key % TABLE_SIZE);  // division-method hash
    for (int step = 0; step < TABLE_SIZE; ++step) {
        int idx = (home + step) % TABLE_SIZE;  // linear probing
        const Slot& slot = m_table[idx];
        if (!slot.used) return idx;
        if (slot.key == key && slot.layout == layout) return idx;
    }
    return -1;
}

std::vector<BenchmarkEntry> Benchmark::run(const Grid& grid, bool& fromCache) {
    GridLayout layout = grid.layout();
    std::uint64_t key = fingerprint(layout);

    int idx = probe(key, layout);
    if (idx >= 0 && m_table[idx].used) {
        fromCache = true;
        return m_table[idx].ranking;
    }
    fromCache = false;

    std::vector<BenchmarkEntry> ranking;
    for (Algorithm algo : {Algorithm::BFS, Algorithm::DFS, Algorithm::Dijkstra, Algorithm::AStar}) {
        SearchResult r = runAlgorithm(algo, grid);
        ranking.push_back({algo, r.nodesVisited, r.found ? pathCost(grid, r.path) : 0, r.found});
    }
    std::vector<BenchmarkEntry> temp(ranking.size());
    mergeSort(ranking, temp, 0, static_cast<int>(ranking.size()));

    // Table full: start over rather than grow, since old mazes are rarely revisited.
    if (idx < 0) {
        for (Slot& slot : m_table) slot = Slot{};
        idx = probe(key, layout);
    }
    m_table[idx] = Slot{true, key, std::move(layout), ranking};
    return ranking;
}
