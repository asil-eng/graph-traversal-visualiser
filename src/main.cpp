#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

#include "Grid.h"
#include "Algorithms.h"
#include "MazeGenerator.h"
#include "EditHistory.h"
#include "Benchmark.h"

namespace {

constexpr float CELL_SIZE = 18.f;
// Starting size; the grid grows or shrinks to fill the window when it is resized.
constexpr int INITIAL_ROWS = 33;
constexpr int INITIAL_COLS = 53;
constexpr int MIN_ROWS = 5;
constexpr int MIN_COLS = 5;
constexpr float HUD_HEIGHT = 88.f;

struct Animation {
    SearchResult result;
    std::size_t visitedIndex = 0;
    std::size_t pathIndex = 0;
    int visitedPerFrame = 1;
    int pathPerFrame = 1;
    bool active = false;
    bool revealingPath = false;
};

void startAnimation(Animation& anim, Grid& grid, Algorithm algo) {
    grid.clearSearchState();
    anim.result = runAlgorithm(algo, grid);
    anim.visitedIndex = 0;
    anim.pathIndex = 0;
    anim.revealingPath = false;
    anim.active = true;
    anim.visitedPerFrame = std::max<std::size_t>(1, anim.result.visitedOrder.size() / 150);
    anim.pathPerFrame = std::max<std::size_t>(1, anim.result.path.size() / 60);
}

void stepAnimation(Animation& anim, Grid& grid) {
    if (!anim.active) return;
    Coord start = grid.start();
    Coord end = grid.end();

    if (!anim.revealingPath) {
        for (int i = 0; i < anim.visitedPerFrame && anim.visitedIndex < anim.result.visitedOrder.size(); ++i) {
            Coord c = anim.result.visitedOrder[anim.visitedIndex++];
            if (c == start || c == end) continue;
            bool weighted = grid.at(c.row, c.col) == CellType::Weighted;
            grid.set(c.row, c.col, weighted ? CellType::VisitedWeighted : CellType::Visited);
        }
        if (anim.visitedIndex >= anim.result.visitedOrder.size()) {
            if (anim.result.found) {
                anim.revealingPath = true;
            } else {
                anim.active = false;
            }
        }
    } else {
        for (int i = 0; i < anim.pathPerFrame && anim.pathIndex < anim.result.path.size(); ++i) {
            Coord c = anim.result.path[anim.pathIndex++];
            if (c == start || c == end) continue;
            bool weighted = grid.at(c.row, c.col) == CellType::VisitedWeighted;
            grid.set(c.row, c.col, weighted ? CellType::PathWeighted : CellType::Path);
        }
        if (anim.pathIndex >= anim.result.path.size()) {
            anim.active = false;
        }
    }
}

// Converts a mouse position in window pixels to a grid cell. Goes through the
// window's view so clicks still land correctly after a resize or full screen.
std::optional<Coord> screenToCoord(sf::Vector2i pixel, const sf::RenderWindow& window, const Grid& grid) {
    sf::Vector2f world = window.mapPixelToCoords(pixel);
    if (world.x < 0.f || world.y < 0.f) return std::nullopt;
    int col = static_cast<int>(world.x / grid.cellSize());
    int row = static_cast<int>(world.y / grid.cellSize());
    if (!grid.inBounds(row, col)) return std::nullopt;
    return Coord{row, col};
}

// How many whole cells fit in a window of this size, leaving room for the HUD.
sf::Vector2i gridSizeFor(sf::Vector2u windowSize) {
    int cols = static_cast<int>(windowSize.x / CELL_SIZE);
    int rows = static_cast<int>((static_cast<float>(windowSize.y) - HUD_HEIGHT) / CELL_SIZE);
    return {std::max(cols, MIN_COLS), std::max(rows, MIN_ROWS)};
}

const char* startCommentary(Algorithm algo) {
    switch (algo) {
        case Algorithm::BFS:      return "BFS: checking literally every neighbour, one ring at a time...";
        case Algorithm::DFS:      return "DFS: no plan, just vibes. Diving in headfirst...";
        case Algorithm::Dijkstra: return "Dijkstra: calculating the cheapest route like a broke student...";
        case Algorithm::AStar:    return "A*: same as Dijkstra but with a GPS...";
    }
    return "Searching...";
}

std::string leaderboardText(const std::vector<BenchmarkEntry>& ranking, bool fromCache) {
    std::string text = "Ranking:";
    for (std::size_t i = 0; i < ranking.size(); ++i) {
        const BenchmarkEntry& e = ranking[i];
        text += "  " + std::to_string(i + 1) + ". " + algorithmName(e.algo) + " (" +
                std::to_string(e.nodesVisited) + " visited, " +
                (e.found ? "cost " + std::to_string(e.pathCost) : std::string("no path")) + ")";
    }
    if (fromCache) text += "  [cached]";
    return text;
}

}  // namespace

int main() {
    Grid grid(INITIAL_ROWS, INITIAL_COLS, CELL_SIZE);

    sf::RenderWindow window(
        sf::VideoMode({static_cast<unsigned>(INITIAL_COLS * CELL_SIZE),
                        static_cast<unsigned>(INITIAL_ROWS * CELL_SIZE + HUD_HEIGHT)}),
        "Pathfinding Visualizer");
    window.setFramerateLimit(60);

    sf::Font font;
    bool haveFont = font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    sf::SoundBuffer blipBuffer, victoryBuffer, failBuffer;
    bool haveSound = blipBuffer.loadFromFile("assets/blip.wav") &&
                      victoryBuffer.loadFromFile("assets/victory.wav") &&
                      failBuffer.loadFromFile("assets/fail.wav");
    std::optional<sf::Sound> blipSound, victorySound, failSound;
    if (haveSound) {
        blipSound.emplace(blipBuffer);
        victorySound.emplace(victoryBuffer);
        failSound.emplace(failBuffer);
        blipSound->setVolume(35.f);
    }

    Algorithm selected = Algorithm::BFS;
    Animation anim;
    bool lastRunFound = true;
    bool haveRunOnce = false;
    std::string commentary;

    bool paintingWall = false;
    bool erasing = false;

    // Key state comes from KeyPressed/KeyReleased events rather than
    // sf::Keyboard::isKeyPressed, which on macOS reports letter keys as up
    // unless the app has been granted Input Monitoring permission.
    enum class Placing { None, Start, End };
    Placing placing = Placing::None;
    bool shiftHeld = false;

    EditHistory history;
    history.record(grid.layout());
    Benchmark benchmark;
    std::string leaderboard;

    // Call after any edit to walls/weights/start/end.
    auto commitEdit = [&] {
        if (history.record(grid.layout())) leaderboard.clear();
    };
    auto applyHistory = [&](const GridLayout* layout, const char* message) {
        if (!layout) return;
        anim.active = false;
        haveRunOnce = false;
        grid.restore(*layout);
        // A snapshot taken at another window size is fitted to the current one.
        sf::Vector2i cells = gridSizeFor(window.getSize());
        grid.resize(cells.y, cells.x);
        leaderboard.clear();
        commentary = message;
    };

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                // Show the window 1:1 (no stretching) and fill it with cells.
                sf::Vector2f size{static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)};
                window.setView(sf::View(sf::FloatRect({0.f, 0.f}, size)));
                sf::Vector2i cells = gridSizeFor(resized->size);
                if (cells.x != grid.cols() || cells.y != grid.rows()) {
                    anim.active = false;
                    haveRunOnce = false;
                    paintingWall = false;
                    erasing = false;
                    grid.clearSearchState();
                    grid.resize(cells.y, cells.x);
                    commitEdit();
                }
            }

            if (event->is<sf::Event::FocusLost>()) {
                shiftHeld = false;
                paintingWall = false;
                erasing = false;
            }

            if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
                if (key->code == sf::Keyboard::Key::LShift || key->code == sf::Keyboard::Key::RShift) {
                    shiftHeld = false;
                }
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                switch (key->code) {
                    case sf::Keyboard::Key::Escape:
                        if (placing != Placing::None) {
                            placing = Placing::None;
                            commentary = "Cancelled.";
                        } else {
                            window.close();
                        }
                        break;
                    case sf::Keyboard::Key::LShift:
                    case sf::Keyboard::Key::RShift:
                        shiftHeld = true;
                        break;
                    case sf::Keyboard::Key::S:
                        placing = Placing::Start;
                        commentary = "Click a cell to place the START (Esc to cancel).";
                        break;
                    case sf::Keyboard::Key::E:
                        placing = Placing::End;
                        commentary = "Click a cell to place the END (Esc to cancel).";
                        break;
                    case sf::Keyboard::Key::Num1: selected = Algorithm::BFS; break;
                    case sf::Keyboard::Key::Num2: selected = Algorithm::DFS; break;
                    case sf::Keyboard::Key::Num3: selected = Algorithm::Dijkstra; break;
                    case sf::Keyboard::Key::Num4: selected = Algorithm::AStar; break;
                    case sf::Keyboard::Key::Enter:
                        startAnimation(anim, grid, selected);
                        haveRunOnce = true;
                        lastRunFound = anim.result.found;
                        commentary = startCommentary(selected);
                        break;
                    case sf::Keyboard::Key::C:
                        anim.active = false;
                        haveRunOnce = false;
                        grid.clearAll();
                        commitEdit();
                        break;
                    case sf::Keyboard::Key::V:
                        anim.active = false;
                        haveRunOnce = false;
                        grid.clearSearchState();
                        break;
                    case sf::Keyboard::Key::M:
                        anim.active = false;
                        haveRunOnce = false;
                        generateMaze(grid);
                        commitEdit();
                        break;
                    case sf::Keyboard::Key::Z:
                        applyHistory(history.undo(), "Undo. Pretend that never happened.");
                        break;
                    case sf::Keyboard::Key::Y:
                        applyHistory(history.redo(), "Redo. Changed your mind again?");
                        break;
                    case sf::Keyboard::Key::B: {
                        anim.active = false;
                        haveRunOnce = false;
                        grid.clearSearchState();
                        bool fromCache = false;
                        auto ranking = benchmark.run(grid, fromCache);
                        leaderboard = leaderboardText(ranking, fromCache);
                        commentary = ranking.front().found
                            ? std::string(algorithmName(ranking.front().algo)) + " wins: fewest cells explored."
                            : "Nobody wins. There is no path.";
                        break;
                    }
                    default: break;
                }
            }

            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                auto coord = screenToCoord(mouse->position, window, grid);
                if (coord) {
                    if (placing != Placing::None && mouse->button == sf::Mouse::Button::Left) {
                        bool isStart = placing == Placing::Start;
                        placing = Placing::None;
                        anim.active = false;
                        haveRunOnce = false;
                        grid.clearSearchState();
                        if (isStart) grid.setStart(*coord); else grid.setEnd(*coord);
                        if ((isStart ? grid.start() : grid.end()) == *coord) {
                            commentary = isStart ? "Start placed." : "End placed.";
                            commitEdit();
                        } else {
                            commentary = "Can't go there: pick a cell that isn't a wall or the other marker.";
                        }
                    } else if (mouse->button == sf::Mouse::Button::Left) {
                        paintingWall = true;
                        bool weighted = shiftHeld;
                        if (*coord != grid.start() && *coord != grid.end()) {
                            grid.set(coord->row, coord->col, weighted ? CellType::Weighted : CellType::Wall);
                        }
                    } else if (mouse->button == sf::Mouse::Button::Right) {
                        erasing = true;
                        if (*coord != grid.start() && *coord != grid.end()) {
                            grid.set(coord->row, coord->col, CellType::Empty);
                        }
                    }
                }
            }

            if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouse->button == sf::Mouse::Button::Left) paintingWall = false;
                if (mouse->button == sf::Mouse::Button::Right) erasing = false;
                commitEdit();
            }

            if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
                if (paintingWall || erasing) {
                    auto coord = screenToCoord(moved->position, window, grid);
                    if (coord && *coord != grid.start() && *coord != grid.end()) {
                        if (paintingWall) {
                            bool weighted = shiftHeld;
                            grid.set(coord->row, coord->col, weighted ? CellType::Weighted : CellType::Wall);
                        } else {
                            grid.set(coord->row, coord->col, CellType::Empty);
                        }
                    }
                }
            }
        }

        bool wasActive = anim.active;
        std::size_t prevVisitedIndex = anim.visitedIndex;
        std::size_t prevPathIndex = anim.pathIndex;

        stepAnimation(anim, grid);

        if (wasActive && haveSound &&
            (anim.visitedIndex != prevVisitedIndex || anim.pathIndex != prevPathIndex)) {
            std::size_t total = anim.result.visitedOrder.size() + anim.result.path.size();
            std::size_t done = anim.visitedIndex + anim.pathIndex;
            float progress = total > 0 ? static_cast<float>(done) / static_cast<float>(total) : 0.f;
            blipSound->setPitch(0.8f + progress * 1.0f);
            blipSound->play();
        }

        if (wasActive && !anim.active) {
            lastRunFound = anim.result.found;
            if (anim.result.found) {
                commentary = "Reached the goal! GPS unlocked.";
                if (haveSound) victorySound->play();
            } else {
                commentary = "No path exists. Somebody call a plumber.";
                if (haveSound) failSound->play();
            }
        }

        window.clear(sf::Color(15, 15, 18));
        grid.draw(window);

        if (haveFont) {
            std::string line1 = "[1] BFS  [2] DFS  [3] Dijkstra  [4] A*   Selected: " +
                                 std::string(algorithmName(selected)) +
                                 "  |  Enter: Run  B: Benchmark  M: Maze  C: Clear All  V: Clear Path";
            std::string line2 = "S then click: Start  E then click: End  L-drag: Wall  Shift+L-drag: Weight  R-drag: Erase  Z/Y: Undo/Redo";
            if (haveRunOnce) {
                line2 += "   |   Visited: " + std::to_string(anim.result.nodesVisited) +
                          "   Path: " + (lastRunFound ? std::to_string(anim.result.path.size()) : std::string("none"));
            }

            sf::Text text1(font, line1, 13);
            text1.setPosition({6.f, grid.rows() * CELL_SIZE + 4.f});
            text1.setFillColor(sf::Color(230, 230, 230));

            sf::Text text2(font, line2, 13);
            text2.setPosition({6.f, grid.rows() * CELL_SIZE + 24.f});
            text2.setFillColor(sf::Color(180, 180, 190));

            window.draw(text1);
            window.draw(text2);

            if (!commentary.empty()) {
                sf::Text text3(font, commentary, 14);
                text3.setStyle(sf::Text::Italic);
                text3.setPosition({6.f, grid.rows() * CELL_SIZE + 46.f});
                text3.setFillColor(sf::Color(240, 210, 60));
                window.draw(text3);
            }

            if (!leaderboard.empty()) {
                sf::Text text4(font, leaderboard, 13);
                text4.setPosition({6.f, grid.rows() * CELL_SIZE + 66.f});
                text4.setFillColor(sf::Color(120, 210, 160));
                window.draw(text4);
            }
        }

        window.display();
    }

    return 0;
}
