#pragma once

#include "Grid.h"
#include <cstddef>

// Undo/redo history implemented as a hand-written doubly linked list of grid
// layouts. `m_current` points at the state the grid is in now; undo walks to
// `prev`, redo walks to `next`. Recording a new state after an undo discards
// the redo branch, just like a browser's back/forward history.
class EditHistory {
public:
    explicit EditHistory(std::size_t capacity = 200);
    ~EditHistory();

    EditHistory(const EditHistory&) = delete;
    EditHistory& operator=(const EditHistory&) = delete;

    // Appends `layout` after the current node. Returns false (and records
    // nothing) if it is identical to the current state.
    bool record(const GridLayout& layout);

    // Move the cursor and return the layout to restore, or nullptr if there
    // is nothing to undo/redo.
    const GridLayout* undo();
    const GridLayout* redo();

    std::size_t size() const { return m_size; }

private:
    struct Node {
        GridLayout layout;
        Node* prev = nullptr;
        Node* next = nullptr;
    };

    void deleteAfter(Node* node);  // frees every node after `node`
    void popFront();               // drops the oldest node when over capacity

    Node* m_head = nullptr;
    Node* m_tail = nullptr;
    Node* m_current = nullptr;
    std::size_t m_size = 0;
    std::size_t m_capacity;
};
