#include "Stack.h"

void push(Stack& s, const Visitor& v) {
    SNode* node = new SNode;
    node->data = v;
    node->next = s.top;
    s.top = node;
    ++s.size;
}

bool pop(Stack& s, Visitor& out) {
    if (s.top == nullptr) {
        return false;
    }
    SNode* node = s.top;
    out = node->data;
    s.top = node->next;
    delete node;
    --s.size;
    return true;
}

bool isEmpty(const Stack& s) {
    return s.top == nullptr;
}

void clear(Stack& s) {
    while (s.top != nullptr) {
        SNode* node = s.top;
        s.top = node->next;
        delete node;
    }
    s.size = 0;
}
