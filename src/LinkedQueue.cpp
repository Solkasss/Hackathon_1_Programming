#include "LinkedQueue.h"

void pushBack(LinkedQueue& q, const Visitor& v) {
    QNode* node = new QNode;
    node->data = v;
    node->next = nullptr;
    if (q.tail == nullptr) {
        q.head = node;
        q.tail = node;
    } else {
        q.tail->next = node;
        q.tail = node;
    }
    ++q.size;
}

void pushFront(LinkedQueue& q, const Visitor& v) {
    QNode* node = new QNode;
    node->data = v;
    node->next = q.head;
    q.head = node;
    if (q.tail == nullptr) {
        q.tail = node;
    }
    ++q.size;
}

bool popFront(LinkedQueue& q, Visitor& out) {
    if (q.head == nullptr) {
        return false;
    }
    QNode* node = q.head;
    out = node->data;
    q.head = node->next;
    if (q.head == nullptr) {
        q.tail = nullptr;
    }
    delete node;
    --q.size;
    return true;
}

bool removeById(LinkedQueue& q, int id) {
    QNode* prev = nullptr;
    QNode* cur = q.head;
    while (cur != nullptr) {
        if (cur->data.id == id) {
            if (prev == nullptr) {
                q.head = cur->next;
            } else {
                prev->next = cur->next;
            }
            if (q.tail == cur) {
                q.tail = prev;
            }
            delete cur;
            --q.size;
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

bool isEmpty(const LinkedQueue& q) {
    return q.head == nullptr;
}

void clear(LinkedQueue& q) {
    while (q.head != nullptr) {
        QNode* node = q.head;
        q.head = node->next;
        delete node;
    }
    q.tail = nullptr;
    q.size = 0;
}
