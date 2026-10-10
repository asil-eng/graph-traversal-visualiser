#pragma once

#include "Grid.h"
#include <vector>

enum class Algorithm { BFS, DFS, Dijkstra, AStar };

struct SearchResult {
    std::vector<Coord> visitedOrder;  // cells in the order the algorithm expanded them (for animation)
    std::vector<Coord> path;          // reconstructed shortest/found path, empty if none
    int nodesVisited = 0;
    bool found = false;
};

SearchResult runBFS(const Grid& grid);
SearchResult runDFS(const Grid& grid);
SearchResult runDijkstra(const Grid& grid);
SearchResult runAStar(const Grid& grid);

SearchResult runAlgorithm(Algorithm algo, const Grid& grid);

const char* algorithmName(Algorithm algo);
