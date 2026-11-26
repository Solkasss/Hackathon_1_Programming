/*
/  Програма з основною поведінкою: 2 черги, стек обслужених,
/  рахунок 3 пільгових - 1 звичайний, клієнт, айді для наступного,
/  enum з можливими результатами
*/

#pragma once
#include "Visitor.h"
#include "LinkedQueue.h"
#include "Stack.h"

enum class Result { // enum з можливими помилками і повідомленнями. Ми збираємось робити і консоль і GUI, тому просто виводити текст не вийде, тому в файлах кидаємо результат з запропонованих варіантів, а далі середовище інтерпретуватиме як хоче 
    OK,
    QueuesEmpty,        // F03: чекати нікого (поточний лишається біля вікна)
    NoCurrent,          // F07, F09: біля вікна нікого немає
    NotWaiting,         // F05, F04: талона немає в чергах
    NameEmpty,          // F01: ім'я порожнє після обрізання пробілів
    NameTooLong,        // F01: довше за 30 символів
    NameBadChars,       // F01: заборонені символи або жодної літери
    DurationOutOfRange, // F07: не в діапазоні 1..240
    NothingToReturn,    // F10: стек обслужених порожній
    NoDurationData      // F08: жодної записаної тривалості для середнього
};

struct DeskState {
    LinkedQueue regular;
    LinkedQueue priority;

    Stack served;

    Visitor current;
    bool hasCurrent = false;

    int priorityStreak = 0;

    int nextId = 1;
};

void destroyDesk(DeskState& d); // Метод, який очистить черги і стек, а потім видалить стан дошки в кінці програми.