#include "Grid.h"

#include <algorithm>

Grid::Grid(int rows, int cols, float cellSize)
    : m_rows(rows), m_cols(cols), m_cellSize(cellSize),
      m_cells(rows, std::vector<CellType>(cols, CellType::Empty)) {
    m_start = {rows / 2, cols / 4};
    m_end = {rows / 2, (3 * cols) / 4};
    m_cells[m_start.row][m_start.col] = CellType::Start;
    m_cells[m_end.row][m_end.col] = CellType::End;
}

bool Grid::inBounds(int row, int col) const {
    return row >= 0 && row < m_rows && col >= 0 && col < m_cols;
}

CellType Grid::at(int row, int col) const {
    return m_cells[row][col];
}

void Grid::set(int row, int col, CellType type) {
    if (!inBounds(row, col)) return;
    m_cells[row][col] = type;
}

int Grid::cost(int row, int col) const {
    if (!inBounds(row, col)) return -1;
    CellType t = m_cells[row][col];
    if (t == CellType::Wall) return -1;
    if (t == CellType::Weighted || t == CellType::VisitedWeighted || t == CellType::PathWeighted) return 5;
    return 1;
}

void Grid::setStart(Coord c) {
    if (!inBounds(c.row, c.col) || m_cells[c.row][c.col] == CellType::Wall) return;
    if (c == m_end) return;
    m_cells[m_start.row][m_start.col] = CellType::Empty;
    m_start = c;
    m_cells[c.row][c.col] = CellType::Start;
}

void Grid::setEnd(Coord c) {
    if (!inBounds(c.row, c.col) || m_cells[c.row][c.col] == CellType::Wall) return;
    if (c == m_start) return;
    m_cells[m_end.row][m_end.col] = CellType::Empty;
    m_end = c;
    m_cells[c.row][c.col] = CellType::End;
}

std::vector<Coord> Grid::neighbors4(Coord c) const {
    static const int dr[] = {-1, 1, 0, 0};
    static const int dc[] = {0, 0, -1, 1};
    std::vector<Coord> result;
    result.reserve(4);
    for (int i = 0; i < 4; ++i) {
        Coord n{c.row + dr[i], c.col + dc[i]};
        if (inBounds(n.row, n.col) && m_cells[n.row][n.col] != CellType::Wall) {
            result.push_back(n);
        }
    }
    return result;
}

void Grid::clearAll() {
    m_cells.assign(m_rows, std::vector<CellType>(m_cols, CellType::Empty));
    m_start = {m_rows / 2, m_cols / 4};
    m_end = {m_rows / 2, (3 * m_cols) / 4};
    m_cells[m_start.row][m_start.col] = CellType::Start;
    m_cells[m_end.row][m_end.col] = CellType::End;
}

void Grid::clearSearchState() {
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            CellType& t = m_cells[r][c];
            if (t == CellType::Frontier || t == CellType::Visited || t == CellType::Path) {
                t = CellType::Empty;
            } else if (t == CellType::VisitedWeighted || t == CellType::PathWeighted) {
                t = CellType::Weighted;
            }
        }
    }
    m_cells[m_start.row][m_start.col] = CellType::Start;
    m_cells[m_end.row][m_end.col] = CellType::End;
}

GridLayout Grid::layout() const {
    GridLayout result{m_cells, m_start, m_end};
    for (auto& row : result.cells) {
        for (CellType& t : row) {
            if (t == CellType::Frontier || t == CellType::Visited || t == CellType::Path) {
                t = CellType::Empty;
            } else if (t == CellType::VisitedWeighted || t == CellType::PathWeighted) {
                t = CellType::Weighted;
            }
        }
    }
    return result;
}

void Grid::restore(const GridLayout& layout) {
    m_cells = layout.cells;
    m_rows = static_cast<int>(m_cells.size());
    m_cols = m_rows > 0 ? static_cast<int>(m_cells[0].size()) : 0;
    m_start = layout.start;
    m_end = layout.end;
}

void Grid::resize(int rows, int cols) {
    if (rows == m_rows && cols == m_cols) return;

    std::vector<std::vector<CellType>> cells(rows, std::vector<CellType>(cols, CellType::Empty));
    for (int r = 0; r < std::min(rows, m_rows); ++r) {
        for (int c = 0; c < std::min(cols, m_cols); ++c) {
            CellType t = m_cells[r][c];
            if (t != CellType::Start && t != CellType::End) cells[r][c] = t;
        }
    }
    m_cells = std::move(cells);
    m_rows = rows;
    m_cols = cols;

    auto clampInside = [&](Coord c) {
        return Coord{std::clamp(c.row, 0, rows - 1), std::clamp(c.col, 0, cols - 1)};
    };
    m_start = clampInside(m_start);
    m_end = clampInside(m_end);
    if (m_end == m_start) {
        // Both were pushed onto the same corner; nudge the end one step away.
        m_end.col = m_start.col > 0 ? m_start.col - 1 : m_start.col + 1;
    }
    m_cells[m_start.row][m_start.col] = CellType::Start;
    m_cells[m_end.row][m_end.col] = CellType::End;
}

static sf::Color colorFor(CellType type) {
    switch (type) {
        case CellType::Empty:    return sf::Color(28, 28, 34);
        case CellType::Wall:     return sf::Color(60, 60, 70);
        case CellType::Weighted: return sf::Color(150, 110, 40);
        case CellType::Start:    return sf::Color(60, 200, 100);
        case CellType::End:      return sf::Color(220, 70, 70);
        case CellType::Frontier: return sf::Color(90, 140, 230);
        case CellType::Visited:  return sf::Color(60, 90, 160);
        case CellType::Path:     return sf::Color(240, 210, 60);
        case CellType::VisitedWeighted: return sf::Color(125, 85, 45);
        case CellType::PathWeighted:    return sf::Color(250, 160, 40);
    }
    return sf::Color::Black;
}

void Grid::draw(sf::RenderWindow& window) const {
    sf::RectangleShape cellShape({m_cellSize - 1.f, m_cellSize - 1.f});

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            cellShape.setPosition({c * m_cellSize, r * m_cellSize});
            cellShape.setFillColor(colorFor(m_cells[r][c]));
            window.draw(cellShape);
        }
    }
}
