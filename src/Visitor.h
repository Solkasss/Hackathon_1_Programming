#pragma once
#include <string>
#include <ostream>

enum class VisitorType {
    REGULAR,
    PRIORITY
};

struct Visitor {
    int id = 0;                                 // Номер талону
    std::string name;                           // Ім'я клієнта
    VisitorType type = VisitorType::REGULAR;    // Тип(Звичайний, пільговий)
    bool hasDuration = false;                   // Прапорець на випадок, якщо час не буде записаний. Такий клієнт не йтиме в статистику часу 
    int duration = 0;                           // час прийому в хвилинах (1-240)
};

std::string typeToStr(VisitorType v); // Перевести enum з типами клієнта в текстовий вивід

std::ostream& operator<<(std::ostream& os, const Visitor& v); // Вивід інформації про клієнта через перевантаження оператора