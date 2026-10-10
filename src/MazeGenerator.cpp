#include "MazeGenerator.h"

#include <random>
#include <stack>
#include <algorithm>
#include <array>

namespace {

bool carveable(int r, int c, int rows, int cols) {
    return r >= 1 && r < rows - 1 && c >= 1 && c < cols - 1;
}

// Opens up a 2x2 block around `c` so an arbitrary (possibly even-aligned)
// cell is guaranteed to touch the odd-aligned maze lattice.
void forceOpen(Grid& grid, Coord c) {
    int rows = grid.rows(), cols = grid.cols();
    int nr = (c.row % 2 == 0) ? c.row + 1 : c.row;
    if (nr >= rows - 1) nr = c.row - 1;
    int nc = (c.col % 2 == 0) ? c.col + 1 : c.col;
    if (nc >= cols - 1) nc = c.col - 1;

    grid.set(c.row, c.col, CellType::Empty);
    if (grid.inBounds(c.row, nc)) grid.set(c.row, nc, CellType::Empty);
    if (grid.inBounds(nr, c.col)) grid.set(nr, c.col, CellType::Empty);
    if (grid.inBounds(nr, nc)) grid.set(nr, nc, CellType::Empty);
}

}  // namespace

void generateMaze(Grid& grid) {
    int rows = grid.rows();
    int cols = grid.cols();

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid.set(r, c, CellType::Wall);
        }
    }

    static std::mt19937 rng(std::random_device{}());
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));

    Coord origin{1, 1};
    grid.set(origin.row, origin.col, CellType::Empty);
    visited[origin.row][origin.col] = true;

    std::stack<Coord> stack;
    stack.push(origin);

    const int dr[] = {-2, 2, 0, 0};
    const int dc[] = {0, 0, -2, 2};

    while (!stack.empty()) {
        Coord cur = stack.top();

        std::array<int, 4> dirs = {0, 1, 2, 3};
        std::shuffle(dirs.begin(), dirs.end(), rng);

        bool carved = false;
        for (int d : dirs) {
            int nr = cur.row + dr[d];
            int nc = cur.col + dc[d];
            if (carveable(nr, nc, rows, cols) && !visited[nr][nc]) {
                int wr = cur.row + dr[d] / 2;
                int wc = cur.col + dc[d] / 2;
                grid.set(wr, wc, CellType::Empty);
                grid.set(nr, nc, CellType::Empty);
                visited[nr][nc] = true;
                stack.push({nr, nc});
                carved = true;
                break;
            }
        }
        if (!carved) stack.pop();
    }

    forceOpen(grid, grid.start());
    forceOpen(grid, grid.end());
    grid.set(grid.start().row, grid.start().col, CellType::Start);
    grid.set(grid.end().row, grid.end().col, CellType::End);
}
