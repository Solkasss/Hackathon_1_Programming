#pragma once
#include "DeskState.h"

Result registerVisitor(DeskState&, const std::string& name, VisitorType, Visitor& out); // зареєструвати клієнта
void showQueues(const DeskState&, std::ostream&);                                      // Показати наповнення обох черг
Result cancelTicket(DeskState&, int id);                                               // Відмінити запис по id
int findVisitors(const DeskState&, const std::string& part, std::ostream&);            // Знайти відвідувачів по імені
Result recordDuration(DeskState&, int minutes);                                        // Записати тривалість відвідування клієнту
Result getStats(const DeskState&, int& served, int& waiting, double& avg);             // Отримати статистику
Result callNext(DeskState&, Visitor& out);                                             // Викликати наступного клієнта з черги відповідно до рахунку
Result peopleAhead(const DeskState&, int id, int& ahead);                              // Знайти кількість людей попереду людии з зазначеним id, записати в int& ahead
Result postponeCurrent(DeskState&, Visitor& out);                                      // Відкласти поточного клієнта
Result returnLastServed(DeskState&, Visitor& out);                                     // Повернути зі стеку останнього обслуженого клієнта