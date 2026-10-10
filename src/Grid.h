#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

enum class CellType {
    Empty,
    Wall,
    Weighted,   // costs more to traverse (used by Dijkstra/A*)
    Start,
    End,
    Frontier,   // currently in the algorithm's open set
    Visited,    // already expanded by the algorithm
    Path,       // part of the final reconstructed path
    // Search overlays on a weighted cell, so the weight survives a run.
    VisitedWeighted,
    PathWeighted
};

struct Coord {
    int row = -1;
    int col = -1;

    bool operator==(const Coord& other) const {
        return row == other.row && col == other.col;
    }
    bool operator!=(const Coord& other) const { return !(*this == other); }
};

// The user-editable part of a grid (walls, weights, start, end) with no
// algorithm state. Used as an undo/redo snapshot and as a benchmark cache key.
struct GridLayout {
    std::vector<std::vector<CellType>> cells;
    Coord start;
    Coord end;

    bool operator==(const GridLayout& other) const = default;
};

class Grid {
public:
    Grid(int rows, int cols, float cellSize);

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    float cellSize() const { return m_cellSize; }

    CellType at(int row, int col) const;
    void set(int row, int col, CellType type);

    bool inBounds(int row, int col) const;
    // Movement cost of entering this cell (1 for normal, 5 for weighted, -1 if impassable).
    int cost(int row, int col) const;

    Coord start() const { return m_start; }
    Coord end() const { return m_end; }
    void setStart(Coord c);
    void setEnd(Coord c);

    std::vector<Coord> neighbors4(Coord c) const;

    // Clears everything back to Empty (including walls, start, end).
    void clearAll();
    // Clears only algorithm state (Frontier/Visited/Path), keeps walls/start/end/weights.
    void clearSearchState();

    // Snapshot of walls/weights/start/end (Frontier/Visited/Path read as Empty).
    GridLayout layout() const;
    void restore(const GridLayout& layout);

    // Changes the grid's dimensions, keeping every cell that still fits and
    // pulling start/end back inside if they would fall off the edge.
    void resize(int rows, int cols);

    void draw(sf::RenderWindow& window) const;

private:
    int m_rows;
    int m_cols;
    float m_cellSize;
    std::vector<std::vector<CellType>> m_cells;
    Coord m_start{0, 0};
    Coord m_end{0, 0};
};
