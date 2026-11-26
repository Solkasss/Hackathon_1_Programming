#pragma once
#include "Visitor.h"

struct SNode {
    Visitor data;
    SNode* next = nullptr;  
};

struct Stack {
    SNode* top = nullptr;
    int size = 0;

    Stack() = default;
    Stack(const Stack&) = delete;            // Забороняє копіювання стеку, для уникнення витоків пам'яті
    Stack& operator=(const Stack&) = delete; // Забороняє присвоєння одного стеку до іншого (b = a), для уникнення витоків пам'яті
};

void push(Stack& s, const Visitor& v);   // Додати клієнта в початок
bool pop(Stack& s, Visitor& out);        // Прибрати верхнього клієнта зі стопки, записати в Visitor& out
bool isEmpty(const Stack& s);            // Перевірити чи стопка порожня, повернути true/false
void clear(Stack& s);                    // Очистити стопку