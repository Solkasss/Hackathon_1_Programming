#pragma once
#include "Visitor.h"

struct QNode {      // Вузол черги, зберігає значення/клієнта, і вказівник на наступного в чергзі.
    Visitor data;   
    QNode* next = nullptr;
};

struct LinkedQueue {         // Черга з вузлів QNode. зберігає перший і останній вузли черги і загальний розмір.
    QNode* head = nullptr;
    QNode* tail = nullptr;
    int size = 0;

    LinkedQueue() = default;
    LinkedQueue(const LinkedQueue&) = delete;            // Забороняє копіювання черги, для уникнення витоків пам'яті
    LinkedQueue& operator=(const LinkedQueue&) = delete; // Забороняє присвоєння однієї черги до іншої (b = a), для уникнення витоків пам'яті
};

void pushBack(LinkedQueue& q, const Visitor& v);     // Додати клієнта в кінець
void pushFront(LinkedQueue& q, const Visitor& v);    // Додати клієнта в початок
bool popFront(LinkedQueue& q, Visitor& out);         // Прибрати першого в черзі, записати його в Visitor& out, повернути true/false в залежності від результату.
bool removeById(LinkedQueue& q, int id);             // Видалити клієнта з черги за номером талона, повернути true/false.
bool isEmpty(const LinkedQueue& q);                  // Перевірити чи черга пуста, повернути true/false
void clear(LinkedQueue& q);                          // Очистити чергу