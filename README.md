# Graph Traversal Visualiser

An interactive C++ app that shows how graph search algorithms find their way through a grid. Draw walls and weighted cells, pick an algorithm, and watch it explore cell by cell before the path it found is drawn.

Built as a Data Structures and Algorithms project (KTU PCCST303).

## Features

- **Four algorithms:** BFS, DFS, Dijkstra and A*, animated step by step
- **Walls and weights:** walls block movement; weighted cells cost 5 instead of 1
- **Maze generator:** random mazes built with a depth-first backtracker
- **Benchmark:** runs all four algorithms on the current grid and ranks them
- **Undo / redo** for every edit
- **Resizable:** in full screen the grid grows to fill the window
- Sound effects while searching

## Data structures used

| Module | Data structure | Where |
|---|---|---|
| 1 | Queue | BFS |
| 1 | Stack | DFS, maze generator |
| 1 | 2D arrays | The grid, distances, parents |
| 2 | Doubly linked list (hand-written) | Undo / redo history (`EditHistory`) |
| 3 | Graph (implicit grid graph) | All searches |
| 3 | Binary heap / priority queue | Dijkstra and A* |
| 4 | Merge sort (hand-written) | Benchmark ranking (`Benchmark`) |
| 4 | Hash table, division method + linear probing (hand-written) | Benchmark result cache (`Benchmark`) |

See [MODULES_2_AND_4.md](MODULES_2_AND_4.md) for a detailed walkthrough of the linked list, merge sort and hash table.

## Building

Requires a C++20 compiler, CMake 3.16+ and [SFML 3](https://www.sfml-dev.org/).

```bash
# macOS
brew install cmake sfml

cmake -S . -B build
cmake --build build
```

Run it from the project folder so it can find the sound files in `assets/`:

```bash
./build/pathfinder
```

The on-screen text uses the macOS system font (`/System/Library/Fonts/Supplemental/Arial.ttf`). On other systems, change that path in `src/main.cpp` to any `.ttf` font.

## Controls

| Key / mouse | Action |
|---|---|
| `1` `2` `3` `4` | Select BFS, DFS, Dijkstra, A* |
| `Enter` | Run the selected algorithm |
| `S`, then click | Place the start |
| `E`, then click | Place the end |
| Left drag | Draw walls |
| `Shift` + left drag | Draw weighted cells |
| Right drag | Erase |
| `M` | Generate a maze |
| `B` | Benchmark all four algorithms |
| `Z` / `Y` | Undo / redo |
| `V` | Clear the search result |
| `C` | Clear everything |
| `Esc` | Cancel placing start/end, or quit |

## Project structure

```
src/
  main.cpp           window, input, animation, status panel
  Grid.h/.cpp        the grid (graph), neighbours, costs, drawing
  Algorithms.h/.cpp  BFS, DFS, Dijkstra, A*, path reconstruction
  MazeGenerator.*    random maze generation
  EditHistory.*      undo/redo with a doubly linked list
  Benchmark.*        merge sort ranking and hash table cache
assets/              sound effects
```

## Testing

All four algorithms were checked against a reference shortest-path solver on 4,000 random grids (walls, weights and mazes) with no failures: every reported path is valid, BFS paths have the fewest steps, and Dijkstra and A* paths have the lowest cost.
