#include "Algorithms.h"

#include <queue>
#include <stack>
#include <limits>
#include <cmath>
#include <algorithm>

namespace {

std::vector<Coord> reconstructPath(const std::vector<std::vector<Coord>>& cameFrom,
                                    Coord start, Coord end) {
    std::vector<Coord> path;
    if (cameFrom[end.row][end.col] == Coord{-1, -1} && end != start) {
        return path;  // no path found
    }
    Coord cur = end;
    while (cur != start) {
        path.push_back(cur);
        cur = cameFrom[cur.row][cur.col];
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

}  // namespace

SearchResult runBFS(const Grid& grid) {
    SearchResult result;
    int rows = grid.rows(), cols = grid.cols();
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<Coord>> cameFrom(rows, std::vector<Coord>(cols, Coord{-1, -1}));

    Coord start = grid.start();
    Coord end = grid.end();

    std::queue<Coord> q;
    q.push(start);
    visited[start.row][start.col] = true;

    while (!q.empty()) {
        Coord cur = q.front();
        q.pop();
        result.visitedOrder.push_back(cur);

        if (cur == end) {
            result.found = true;
            break;
        }
        for (const Coord& n : grid.neighbors4(cur)) {
            if (!visited[n.row][n.col]) {
                visited[n.row][n.col] = true;
                cameFrom[n.row][n.col] = cur;
                q.push(n);
            }
        }
    }

    result.nodesVisited = static_cast<int>(result.visitedOrder.size());
    if (result.found) result.path = reconstructPath(cameFrom, start, end);
    return result;
}

SearchResult runDFS(const Grid& grid) {
    SearchResult result;
    int rows = grid.rows(), cols = grid.cols();
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<Coord>> cameFrom(rows, std::vector<Coord>(cols, Coord{-1, -1}));

    Coord start = grid.start();
    Coord end = grid.end();

    // Each entry carries the cell it was pushed from, so the parent is fixed
    // when the cell is actually visited and the path follows the DFS tree.
    struct Frame { Coord cell; Coord parent; };
    std::stack<Frame> s;
    s.push({start, Coord{-1, -1}});

    while (!s.empty()) {
        Frame top = s.top();
        s.pop();
        Coord cur = top.cell;

        if (visited[cur.row][cur.col]) continue;
        visited[cur.row][cur.col] = true;
        cameFrom[cur.row][cur.col] = top.parent;
        result.visitedOrder.push_back(cur);

        if (cur == end) {
            result.found = true;
            break;
        }
        for (const Coord& n : grid.neighbors4(cur)) {
            if (!visited[n.row][n.col]) {
                s.push({n, cur});
            }
        }
    }

    result.nodesVisited = static_cast<int>(result.visitedOrder.size());
    if (result.found) result.path = reconstructPath(cameFrom, start, end);
    return result;
}

namespace {

struct PQEntry {
    int priority;
    Coord coord;
    bool operator>(const PQEntry& other) const { return priority > other.priority; }
};

int manhattan(Coord a, Coord b) {
    return std::abs(a.row - b.row) + std::abs(a.col - b.col);
}

SearchResult weightedSearch(const Grid& grid, bool useHeuristic) {
    SearchResult result;
    int rows = grid.rows(), cols = grid.cols();
    const int INF = std::numeric_limits<int>::max();

    std::vector<std::vector<int>> dist(rows, std::vector<int>(cols, INF));
    std::vector<std::vector<bool>> settled(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<Coord>> cameFrom(rows, std::vector<Coord>(cols, Coord{-1, -1}));

    Coord start = grid.start();
    Coord end = grid.end();

    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;
    dist[start.row][start.col] = 0;
    pq.push({useHeuristic ? manhattan(start, end) : 0, start});

    while (!pq.empty()) {
        PQEntry top = pq.top();
        pq.pop();
        Coord cur = top.coord;

        if (settled[cur.row][cur.col]) continue;
        settled[cur.row][cur.col] = true;
        result.visitedOrder.push_back(cur);

        if (cur == end) {
            result.found = true;
            break;
        }

        for (const Coord& n : grid.neighbors4(cur)) {
            int stepCost = grid.cost(n.row, n.col);
            if (stepCost < 0 || settled[n.row][n.col]) continue;

            int newDist = dist[cur.row][cur.col] + stepCost;
            if (newDist < dist[n.row][n.col]) {
                dist[n.row][n.col] = newDist;
                cameFrom[n.row][n.col] = cur;
                int priority = newDist + (useHeuristic ? manhattan(n, end) : 0);
                pq.push({priority, n});
            }
        }
    }

    result.nodesVisited = static_cast<int>(result.visitedOrder.size());
    if (result.found) result.path = reconstructPath(cameFrom, start, end);
    return result;
}

}  // namespace

SearchResult runDijkstra(const Grid& grid) {
    return weightedSearch(grid, /*useHeuristic=*/false);
}

SearchResult runAStar(const Grid& grid) {
    return weightedSearch(grid, /*useHeuristic=*/true);
}

SearchResult runAlgorithm(Algorithm algo, const Grid& grid) {
    switch (algo) {
        case Algorithm::BFS:      return runBFS(grid);
        case Algorithm::DFS:      return runDFS(grid);
        case Algorithm::Dijkstra: return runDijkstra(grid);
        case Algorithm::AStar:    return runAStar(grid);
    }
    return {};
}

const char* algorithmName(Algorithm algo) {
    switch (algo) {
        case Algorithm::BFS:      return "BFS";
        case Algorithm::DFS:      return "DFS";
        case Algorithm::Dijkstra: return "Dijkstra";
        case Algorithm::AStar:    return "A*";
    }
    return "?";
}
