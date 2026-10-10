#include "EditHistory.h"

EditHistory::EditHistory(std::size_t capacity) : m_capacity(capacity < 2 ? 2 : capacity) {}

EditHistory::~EditHistory() {
    Node* cur = m_head;
    while (cur) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
}

void EditHistory::deleteAfter(Node* node) {
    Node* cur = node->next;
    node->next = nullptr;
    m_tail = node;
    while (cur) {
        Node* next = cur->next;
        delete cur;
        --m_size;
        cur = next;
    }
}

void EditHistory::popFront() {
    Node* old = m_head;
    m_head = old->next;
    m_head->prev = nullptr;
    delete old;
    --m_size;
}

bool EditHistory::record(const GridLayout& layout) {
    if (m_current && m_current->layout == layout) return false;

    // A new edit after some undos invalidates the redo branch.
    if (m_current) deleteAfter(m_current);

    Node* node = new Node{layout, m_tail, nullptr};
    if (m_tail) {
        m_tail->next = node;
    } else {
        m_head = node;
    }
    m_tail = node;
    m_current = node;
    ++m_size;

    if (m_size > m_capacity) popFront();
    return true;
}

const GridLayout* EditHistory::undo() {
    if (!m_current || !m_current->prev) return nullptr;
    m_current = m_current->prev;
    return &m_current->layout;
}

const GridLayout* EditHistory::redo() {
    if (!m_current || !m_current->next) return nullptr;
    m_current = m_current->next;
    return &m_current->layout;
}
