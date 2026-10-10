#pragma once

#include "Grid.h"

// Carves a maze into the grid using a randomized DFS (recursive backtracker),
// operating on a half-resolution lattice so passages are one cell wide with
// walls between them. Leaves the grid's current start/end cells open and
// connected to the maze.
void generateMaze(Grid& grid);
