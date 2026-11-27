// Тести ядра Queue Desk. Збірка і запуск: див. tests/README у повідомленні до PR
// або команду в кінці цього коментаря.
//   g++ -std=c++17 -Wall -Wextra -fsanitize=address -Isrc -o tests_run tests/test_core.cpp $(ls src/*.cpp | grep -v main.cpp)
//   ./tests_run
//
// Тести написані за специфікацією QD-SPEC-001 і уточненнями замовника.
// Рішення, яких специфікація не дає, у тестах НЕ закріплені (позначені ПИТАННЯ):
//   - пошук порожнього рядка;
//   - порядок помилок recordDuration, коли немає поточного І тривалість некоректна;
//   - округлення середнього на межі .x5 (наприклад, 1.25).

#include <climits>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

#include "Operations.h"
#include "Validation.h"

// ============================================================================
// Каркас
// ============================================================================

static int g_passed = 0, g_failed = 0;
static const char* g_test = "";

#define CHECK(cond) do { \
    if (cond) ++g_passed; \
    else { ++g_failed; std::cout << "FAIL [" << g_test << "] " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; } \
} while (0)

#define RUN(fn) do { g_test = #fn; fn(); } while (0)

// Порівняння черги з очікуваним списком id (для порожньої черги isEmpty)
#define QUEUE_IS(q, ...) do { \
    const int e_[] = {__VA_ARGS__}; \
    CHECK(queueIs((q), e_, (int)(sizeof(e_) / sizeof(e_[0])))); \
} while (0)

// ============================================================================
// Допоміжні функції
// ============================================================================

static Visitor mk(int id) {
    Visitor v;
    v.id = id;
    return v;
}

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

static bool queueIs(const LinkedQueue& q, const int* ids, int n) {
    if (q.size != n) return false;
    const QNode* p = q.head;
    for (int i = 0; i < n; ++i, p = p->next) {
        if (!p || p->data.id != ids[i]) return false;
    }
    return p == nullptr;
}

// size збігається з реальною кількістю вузлів, tail вказує на справді останній
static bool queueConsistent(const LinkedQueue& q) {
    int n = 0;
    const QNode* last = nullptr;
    for (const QNode* p = q.head; p; p = p->next) { ++n; last = p; }
    return n == q.size && q.tail == last;
}

static bool stackConsistent(const Stack& s) {
    int n = 0;
    for (const SNode* p = s.top; p; p = p->next) ++n;
    return n == s.size;
}

// Інваріанти, які мають виконуватись після будь-якої операції
static void invariant(const DeskState& d) {
    CHECK(queueConsistent(d.regular));
    CHECK(queueConsistent(d.priority));
    CHECK(stackConsistent(d.served));

    // усі id унікальні: черги + стек + поточний
    int total = d.regular.size + d.priority.size + d.served.size + (d.hasCurrent ? 1 : 0);
    int* ids = new int[total > 0 ? total : 1];
    int k = 0;
    for (const QNode* p = d.regular.head; p; p = p->next) ids[k++] = p->data.id;
    for (const QNode* p = d.priority.head; p; p = p->next) ids[k++] = p->data.id;
    for (const SNode* p = d.served.top; p; p = p->next) ids[k++] = p->data.id;
    if (d.hasCurrent) ids[k++] = d.current.id;
    bool unique = (k == total);
    for (int i = 0; i < k && unique; ++i)
        for (int j = i + 1; j < k; ++j)
            if (ids[i] == ids[j]) { unique = false; break; }
    delete[] ids;
    CHECK(unique);
}

static Visitor reg(DeskState& d, const char* name, VisitorType t) {
    Visitor v;
    Result r = registerVisitor(d, name, t, v);
    CHECK(r == Result::OK);
    return v;
}

// Талони з контрольного сценарію (таблиця 7 специфікації)
static void registerSeven(DeskState& d) {
    reg(d, "Ivan",   VisitorType::REGULAR);   // 1
    reg(d, "Olena",  VisitorType::PRIORITY);  // 2
    reg(d, "Petro",  VisitorType::REGULAR);   // 3
    reg(d, "Maria",  VisitorType::PRIORITY);  // 4
    reg(d, "Oksana", VisitorType::PRIORITY);  // 5
    reg(d, "Taras",  VisitorType::PRIORITY);  // 6
    reg(d, "Anna",   VisitorType::REGULAR);   // 7
}

static Visitor call(DeskState& d) {
    Visitor v;
    Result r = callNext(d, v);
    CHECK(r == Result::OK);
    return v;
}

static void rec(DeskState& d, int minutes) {
    CHECK(recordDuration(d, minutes) == Result::OK);
}

static Result vn(const char* s) {
    std::string t(s);
    return validateName(t);
}

static int servedTopId(const DeskState& d) {
    return d.served.top ? d.served.top->data.id : -1;
}

static int findCount(const DeskState& d, const char* part, std::string* text = nullptr) {
    std::ostringstream out;
    int n = findVisitors(d, part, out);
    if (text) *text = out.str();
    return n;
}

// ============================================================================
// 1. LinkedQueue
// ============================================================================

static void test_queue_order() {
    LinkedQueue q;
    pushBack(q, mk(1)); pushBack(q, mk(2)); pushBack(q, mk(3));
    pushFront(q, mk(0));
    QUEUE_IS(q, 0, 1, 2, 3);

    Visitor out;
    for (int i = 0; i < 4; ++i) {
        CHECK(popFront(q, out));
        CHECK(out.id == i);
    }
    CHECK(!popFront(q, out));
    CHECK(isEmpty(q) && q.head == nullptr && q.tail == nullptr && q.size == 0);
}

static void test_queue_remove_head_middle_tail() {
    LinkedQueue q;
    for (int i = 1; i <= 5; ++i) pushBack(q, mk(i));

    CHECK(removeById(q, 3)); QUEUE_IS(q, 1, 2, 4, 5);   // середина
    CHECK(removeById(q, 1)); QUEUE_IS(q, 2, 4, 5);      // голова
    CHECK(removeById(q, 5)); QUEUE_IS(q, 2, 4);         // хвіст
    CHECK(q.tail && q.tail->data.id == 4);              // tail оновився
    pushBack(q, mk(9));
    QUEUE_IS(q, 2, 4, 9);                               // pushBack після видалення хвоста
    CHECK(!removeById(q, 42));                          // неіснуючий номер
    QUEUE_IS(q, 2, 4, 9);
    CHECK(queueConsistent(q));
    clear(q);
}

static void test_queue_single_element() {
    LinkedQueue q;
    pushBack(q, mk(1));
    CHECK(removeById(q, 1));
    CHECK(isEmpty(q) && q.head == nullptr && q.tail == nullptr && q.size == 0);

    pushBack(q, mk(2));
    QUEUE_IS(q, 2);
    Visitor out;
    CHECK(popFront(q, out) && out.id == 2);
    CHECK(q.head == nullptr && q.tail == nullptr);       // tail скинутий

    pushFront(q, mk(7));                                 // pushFront у порожню виставляє і head, і tail
    pushBack(q, mk(8));
    QUEUE_IS(q, 7, 8);
    clear(q);
}

static void test_queue_clear() {
    LinkedQueue q;
    clear(q);                                            // clear порожньої
    CHECK(isEmpty(q) && q.tail == nullptr && q.size == 0);

    for (int i = 1; i <= 10; ++i) pushBack(q, mk(i));
    clear(q);
    CHECK(isEmpty(q) && q.head == nullptr && q.tail == nullptr && q.size == 0);
    clear(q);                                            // повторний clear

    pushBack(q, mk(5));                                  // повторне використання
    QUEUE_IS(q, 5);
    clear(q);
}

static void test_queue_many() {
    LinkedQueue q;
    for (int i = 0; i < 1000; ++i) pushBack(q, mk(i));
    CHECK(q.size == 1000 && queueConsistent(q));
    Visitor out;
    bool order = true;
    for (int i = 0; i < 1000; ++i) {
        if (!popFront(q, out) || out.id != i) order = false;
    }
    CHECK(order);
    CHECK(isEmpty(q) && q.tail == nullptr);
    for (int i = 0; i < 1000; ++i) pushFront(q, mk(i));
    CHECK(q.size == 1000 && queueConsistent(q));
    clear(q);
}

// ============================================================================
// 2. Stack
// ============================================================================

static void test_stack_lifo() {
    Stack s;
    push(s, mk(1)); push(s, mk(2)); push(s, mk(3));
    CHECK(s.size == 3 && stackConsistent(s));

    Visitor out;
    CHECK(pop(s, out) && out.id == 3);
    CHECK(pop(s, out) && out.id == 2);
    CHECK(pop(s, out) && out.id == 1);
    CHECK(!pop(s, out));
    CHECK(isEmpty(s) && s.top == nullptr && s.size == 0);
}

static void test_stack_clear() {
    Stack s;
    clear(s);                                            // порожній
    CHECK(isEmpty(s) && s.size == 0);
    for (int i = 0; i < 10; ++i) push(s, mk(i));
    clear(s);
    CHECK(isEmpty(s) && s.top == nullptr && s.size == 0);
    clear(s);                                            // повторно
    push(s, mk(1));                                      // повторне використання
    CHECK(s.size == 1 && s.top && s.top->data.id == 1);
    clear(s);
}

static void test_stack_many() {
    Stack s;
    for (int i = 0; i < 1000; ++i) push(s, mk(i));
    CHECK(s.size == 1000 && stackConsistent(s));
    Visitor out;
    bool order = true;
    for (int i = 999; i >= 0; --i) {
        if (!pop(s, out) || out.id != i) order = false;
    }
    CHECK(order);
    CHECK(isEmpty(s));
}

// ============================================================================
// 3. Visitor: вивід
// ============================================================================

static void test_visitor_output() {
    Visitor v;
    v.id = 3; v.name = "Petro"; v.type = VisitorType::REGULAR;
    std::ostringstream out;
    out << v;
    CHECK(out.str() == "#3 Petro (REGULAR)");

    v.type = VisitorType::PRIORITY;
    CHECK(typeToStr(VisitorType::REGULAR) == "REGULAR");
    CHECK(typeToStr(VisitorType::PRIORITY) == "PRIORITY");

    std::ostringstream out2;
    out2 << v << "|" << v;                               // ланцюжок працює
    CHECK(out2.str() == "#3 Petro (PRIORITY)|#3 Petro (PRIORITY)");
}

// ============================================================================
// 4. Валідація
// ============================================================================

static void test_validate_name_ok() {
    CHECK(vn("Ivan Petrenko") == Result::OK);
    CHECK(vn("Anna-Maria") == Result::OK);
    CHECK(vn("O'Connor") == Result::OK);
    CHECK(vn("A") == Result::OK);                        // мінімум: 1 символ
    CHECK(vn(std::string(30, 'a').c_str()) == Result::OK);       // рівно 30
    CHECK(vn("Ivan   Petrenko") == Result::OK);          // кілька пробілів усередині
    CHECK(vn("-Ivan") == Result::OK);
    CHECK(vn("Ivan-") == Result::OK);

    std::string s = "  Ivan  ";
    CHECK(validateName(s) == Result::OK);
    CHECK(s == "Ivan");                                  // обрізання пробілів по краях

    std::string edge = "  " + std::string(30, 'b') + "  ";     // 30 після обрізання
    CHECK(validateName(edge) == Result::OK);
    CHECK(edge.size() == 30);
}

static void test_validate_name_empty() {
    CHECK(vn("") == Result::NameEmpty);
    CHECK(vn("   ") == Result::NameEmpty);
}

static void test_validate_name_too_long() {
    CHECK(vn(std::string(31, 'a').c_str()) == Result::NameTooLong);
    std::string s = "  " + std::string(31, 'a') + "  ";
    CHECK(validateName(s) == Result::NameTooLong);       // 31 після обрізання
}

static void test_validate_name_bad_chars() {
    CHECK(vn("Ivan1") == Result::NameBadChars);
    CHECK(vn("R2D2") == Result::NameBadChars);
    CHECK(vn("Іван") == Result::NameBadChars);           // кирилиця
    CHECK(vn("\xFF") == Result::NameBadChars);           // байт < 0: isalpha(char) був би UB
    CHECK(vn("Iv\xFF" "n") == Result::NameBadChars);
    CHECK(vn("Iv@n") == Result::NameBadChars);
    CHECK(vn("Ivan\tPetrenko") == Result::NameBadChars); // табуляція не дозволена
    CHECK(vn("Ivan.") == Result::NameBadChars);
    // без жодної літери
    CHECK(vn("-") == Result::NameBadChars);
    CHECK(vn("'") == Result::NameBadChars);
    CHECK(vn(" - ") == Result::NameBadChars);
    CHECK(vn("-'-") == Result::NameBadChars);
}

static void test_validate_duration() {
    CHECK(validateDuration(1) == Result::OK);
    CHECK(validateDuration(120) == Result::OK);
    CHECK(validateDuration(240) == Result::OK);
    CHECK(validateDuration(0) == Result::DurationOutOfRange);
    CHECK(validateDuration(-1) == Result::DurationOutOfRange);
    CHECK(validateDuration(241) == Result::DurationOutOfRange);
    CHECK(validateDuration(INT_MIN) == Result::DurationOutOfRange);
    CHECK(validateDuration(INT_MAX) == Result::DurationOutOfRange);
}

// ============================================================================
// 5. F01: реєстрація
// ============================================================================

static void test_register_ids_and_queues() {
    DeskState d;
    Visitor a = reg(d, "Ivan", VisitorType::REGULAR);
    Visitor b = reg(d, "Olena", VisitorType::PRIORITY);
    Visitor c = reg(d, "Ivan", VisitorType::REGULAR);    // однакові імена дозволені
    CHECK(a.id == 1 && b.id == 2 && c.id == 3);
    CHECK(a.name == "Ivan" && a.type == VisitorType::REGULAR);
    CHECK(b.type == VisitorType::PRIORITY);
    CHECK(!a.hasDuration);
    QUEUE_IS(d.regular, 1, 3);
    QUEUE_IS(d.priority, 2);
    invariant(d);
    destroyDesk(d);
}

static void test_register_trims_name() {
    DeskState d;
    Visitor v = reg(d, "  Ivan  ", VisitorType::REGULAR);
    CHECK(v.name == "Ivan");
    CHECK(d.regular.head && d.regular.head->data.name == "Ivan");
    destroyDesk(d);
}

static void test_register_invalid_does_not_burn_id() {
    DeskState d;
    Visitor v;
    CHECK(registerVisitor(d, "Ivan1", VisitorType::REGULAR, v) == Result::NameBadChars);
    CHECK(registerVisitor(d, "", VisitorType::REGULAR, v) == Result::NameEmpty);
    CHECK(registerVisitor(d, std::string(31, 'a'), VisitorType::PRIORITY, v) == Result::NameTooLong);
    CHECK(isEmpty(d.regular) && isEmpty(d.priority));
    CHECK(d.nextId == 1);

    Visitor ok = reg(d, "Ivan", VisitorType::REGULAR);
    CHECK(ok.id == 1);                                   // номер не втрачений
    invariant(d);
    destroyDesk(d);
}

static void test_register_many() {
    DeskState d;
    for (int i = 0; i < 1000; ++i) reg(d, "Vi", (i % 2) ? VisitorType::REGULAR : VisitorType::PRIORITY);
    CHECK(d.regular.size == 500 && d.priority.size == 500);
    CHECK(d.nextId == 1001);
    invariant(d);
    destroyDesk(d);
}

// ============================================================================
// 6. F02: перегляд
// ============================================================================

static void test_show_queues() {
    DeskState d;
    std::ostringstream empty;
    showQueues(d, empty);
    CHECK(!empty.str().empty());                         // порожні черги: зрозуміле повідомлення

    registerSeven(d);
    std::ostringstream out;
    showQueues(d, out);
    std::string s = out.str();
    CHECK(s.find("Ivan") != std::string::npos);
    CHECK(s.find("Olena") != std::string::npos);
    CHECK(s.find("Anna") != std::string::npos);
    invariant(d);                                        // перегляд нічого не змінює
    CHECK(d.regular.size == 3 && d.priority.size == 4);
    destroyDesk(d);
}

// ============================================================================
// 7. F03: виклик
// ============================================================================

static void test_call_order_table7() {
    DeskState d;
    registerSeven(d);
    const int expect[] = {2, 4, 5, 1, 6, 3, 7};
    for (int i = 0; i < 7; ++i) {
        Visitor v;
        CHECK(callNext(d, v) == Result::OK);
        CHECK(v.id == expect[i]);
    }
    CHECK(d.hasCurrent && d.current.id == 7);
    CHECK(d.served.size == 6);
    CHECK(servedTopId(d) == 3);
    for (const SNode* p = d.served.top; p; p = p->next) CHECK(!p->data.hasDuration);
    CHECK(isEmpty(d.regular) && isEmpty(d.priority));
    invariant(d);
    destroyDesk(d);
}

static void test_call_empty_queues() {
    DeskState d;
    Visitor v;
    CHECK(callNext(d, v) == Result::QueuesEmpty);
    CHECK(!d.hasCurrent);
    CHECK(d.served.size == 0);
    CHECK(d.priorityStreak == 0);
    invariant(d);
    destroyDesk(d);
}

static void test_call_first_call_pushes_nothing() {
    DeskState d;
    reg(d, "Ivan", VisitorType::PRIORITY);
    Visitor v = call(d);
    CHECK(v.id == 1);
    CHECK(d.hasCurrent && d.current.id == 1);
    CHECK(d.served.size == 0);                           // «порожній» попередній у стек не потрапив
    invariant(d);
    destroyDesk(d);
}

static void test_call_empty_with_current_keeps_everything() {
    DeskState d;
    reg(d, "Ivan", VisitorType::PRIORITY);
    call(d);
    int streak = d.priorityStreak;

    Visitor v;
    CHECK(callNext(d, v) == Result::QueuesEmpty);
    CHECK(d.hasCurrent && d.current.id == 1);            // лишився біля вікна
    CHECK(d.served.size == 0);                           // і не став обслуженим
    CHECK(d.priorityStreak == streak);                   // лічильник не змінився
    invariant(d);
    destroyDesk(d);
}

static void test_call_streak_counter() {
    DeskState d;
    for (int i = 0; i < 4; ++i) reg(d, "Pr", VisitorType::PRIORITY);   // 1..4
    reg(d, "Rg", VisitorType::REGULAR);                                // 5

    call(d); CHECK(d.priorityStreak == 1);
    call(d); CHECK(d.priorityStreak == 2);
    call(d); CHECK(d.priorityStreak == 3);
    Visitor v = call(d);
    CHECK(v.id == 5 && v.type == VisitorType::REGULAR);  // після трьох пільгових іде звичайний
    CHECK(d.priorityStreak == 0);                        // будь-який REGULAR скидає
    v = call(d);
    CHECK(v.id == 4);                                    // тоді знову пільговий
    invariant(d);
    destroyDesk(d);
}

static void test_call_priority_empty_resets_counter() {
    DeskState d;
    reg(d, "Pr", VisitorType::PRIORITY);                 // 1
    reg(d, "Rg", VisitorType::REGULAR);                  // 2
    reg(d, "Rg", VisitorType::REGULAR);                  // 3

    Visitor v = call(d);
    CHECK(v.id == 1 && d.priorityStreak == 1);
    v = call(d);                                         // пільгова порожня, але лічильник лише 1
    CHECK(v.id == 2 && v.type == VisitorType::REGULAR);
    CHECK(d.priorityStreak == 0);                        // REGULAR скинув лічильник
    invariant(d);
    destroyDesk(d);
}

static void test_call_streak_continues_without_regular() {
    DeskState d;
    for (int i = 0; i < 5; ++i) reg(d, "Pr", VisitorType::PRIORITY);   // 1..5
    for (int i = 0; i < 5; ++i) {
        Visitor v = call(d);
        CHECK(v.type == VisitorType::PRIORITY);          // звичайних немає, пільгові йдуть далі
    }
    CHECK(d.priorityStreak >= 3);

    reg(d, "Rg", VisitorType::REGULAR);                  // 6
    Visitor v = call(d);
    CHECK(v.id == 6);                                    // щойно з'явився REGULAR, він наступний
    CHECK(d.priorityStreak == 0);
    invariant(d);
    destroyDesk(d);
}

static void test_call_postponed_counts_as_call() {
    DeskState d;
    reg(d, "Pr", VisitorType::PRIORITY);                 // 1
    reg(d, "Pr", VisitorType::PRIORITY);                 // 2
    reg(d, "Pr", VisitorType::PRIORITY);                 // 3
    reg(d, "Rg", VisitorType::REGULAR);                  // 4

    Visitor v = call(d);
    CHECK(v.id == 1 && d.priorityStreak == 1);
    Visitor p;
    CHECK(postponeCurrent(d, p) == Result::OK);
    CHECK(d.priorityStreak == 1);                        // відкладення лічильник не повертає
    QUEUE_IS(d.priority, 2, 3, 1);

    CHECK(call(d).id == 2);
    CHECK(call(d).id == 3);
    CHECK(d.priorityStreak == 3);
    CHECK(call(d).id == 4);                              // третій пільговий вже був, час звичайного
    CHECK(call(d).id == 1);                              // відкладений пільговий знову в черзі
    invariant(d);
    destroyDesk(d);
}

static void test_call_stress() {
    DeskState d;
    for (int i = 1; i <= 500; ++i) reg(d, "Vi", (i % 3 == 0) ? VisitorType::REGULAR : VisitorType::PRIORITY);

    bool rule = true;
    for (int i = 0; i < 500; ++i) {
        bool regularWaiting = d.regular.size > 0;
        int streakBefore = d.priorityStreak;
        Visitor v;
        if (callNext(d, v) != Result::OK) { rule = false; break; }
        // якщо вже були 3 пільгових і звичайні чекають, виклик має бути звичайним
        if (streakBefore >= 3 && regularWaiting && v.type != VisitorType::REGULAR) rule = false;
    }
    CHECK(rule);
    CHECK(d.served.size == 499 && d.hasCurrent);
    Visitor v;
    CHECK(callNext(d, v) == Result::QueuesEmpty);
    invariant(d);
    destroyDesk(d);
}

// ============================================================================
// 8. F04: людей попереду
// ============================================================================

static int ahead(const DeskState& d, int id, Result* res = nullptr) {
    int a = -999;
    Result r = peopleAhead(d, id, a);
    if (res) *res = r;
    return a;
}

static void test_ahead_table7() {
    DeskState d;
    registerSeven(d);                                    // порядок викликів 2,4,5,1,6,3,7
    CHECK(ahead(d, 1) == 3);
    CHECK(ahead(d, 7) == 6);
    CHECK(ahead(d, 2) == 0);
    CHECK(ahead(d, 6) == 4);
    CHECK(ahead(d, 3) == 5);
    destroyDesk(d);
}

static void test_ahead_after_cancel() {
    DeskState d;
    registerSeven(d);
    CHECK(cancelTicket(d, 5) == Result::OK);             // порядок: 2,4,6,1,3,7
    CHECK(ahead(d, 6) == 2);
    CHECK(ahead(d, 1) == 3);
    CHECK(ahead(d, 7) == 5);
    destroyDesk(d);
}

static void test_ahead_not_waiting() {
    DeskState d;
    registerSeven(d);
    Result r;
    ahead(d, 0, &r);   CHECK(r == Result::NotWaiting);
    ahead(d, -1, &r);  CHECK(r == Result::NotWaiting);
    ahead(d, 99, &r);  CHECK(r == Result::NotWaiting);
    ahead(d, INT_MAX, &r); CHECK(r == Result::NotWaiting);

    call(d);                                             // талон 2 біля вікна
    ahead(d, 2, &r);   CHECK(r == Result::NotWaiting);   // поточного в чергах немає
    destroyDesk(d);
}

static void test_ahead_current_not_counted() {
    DeskState d;
    registerSeven(d);
    call(d);                                             // 2 біля вікна, лічильник 1
    // далі: 4, 5, потім 1 (лічильник дійде до 3)
    CHECK(ahead(d, 1) == 2);
    destroyDesk(d);
}

static void test_ahead_respects_streak() {
    DeskState d;
    for (int i = 0; i < 4; ++i) reg(d, "Pr", VisitorType::PRIORITY);   // 1..4
    reg(d, "Rg", VisitorType::REGULAR);                                // 5
    call(d); call(d);                                    // лічильник 2, поточний 2
    // далі: 3 (лічильник 3), потім звичайний 5, потім 4
    CHECK(ahead(d, 3) == 0);
    CHECK(ahead(d, 5) == 1);
    CHECK(ahead(d, 4) == 2);
    destroyDesk(d);
}

static void test_ahead_simple_cases() {
    DeskState d;
    reg(d, "Solo", VisitorType::REGULAR);
    CHECK(ahead(d, 1) == 0);                             // єдиний
    destroyDesk(d);

    DeskState p;
    for (int i = 0; i < 5; ++i) reg(p, "Pr", VisitorType::PRIORITY);
    for (int id = 1; id <= 5; ++id) CHECK(ahead(p, id) == id - 1);     // лише пільгові
    destroyDesk(p);

    DeskState r;
    for (int i = 0; i < 5; ++i) reg(r, "Rg", VisitorType::REGULAR);
    for (int id = 1; id <= 5; ++id) CHECK(ahead(r, id) == id - 1);     // лише звичайні
    destroyDesk(r);
}

static void test_ahead_is_readonly() {
    DeskState d;
    registerSeven(d);
    int pr = d.priority.size, rg = d.regular.size, streak = d.priorityStreak;
    ahead(d, 7); ahead(d, 99);
    CHECK(d.priority.size == pr && d.regular.size == rg && d.priorityStreak == streak);
    QUEUE_IS(d.priority, 2, 4, 5, 6);
    invariant(d);
    destroyDesk(d);
}

static void test_ahead_after_return() {
    DeskState d;
    registerSeven(d);
    call(d);                                             // 2
    rec(d, 5);                                           // 2 обслужено
    CHECK(ahead(d, 4) == 0);                             // 4 тепер перший
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);       // 2 знову в голові пільгової
    CHECK(ahead(d, 2) == 0);
    CHECK(ahead(d, 4) == 1);
    destroyDesk(d);
}

// ============================================================================
// 9. F05: скасування
// ============================================================================

static void test_cancel_head_middle_tail() {
    DeskState d;
    registerSeven(d);
    CHECK(cancelTicket(d, 2) == Result::OK); QUEUE_IS(d.priority, 4, 5, 6);   // голова
    CHECK(cancelTicket(d, 5) == Result::OK); QUEUE_IS(d.priority, 4, 6);      // середина
    CHECK(cancelTicket(d, 6) == Result::OK); QUEUE_IS(d.priority, 4);         // хвіст
    CHECK(cancelTicket(d, 7) == Result::OK); QUEUE_IS(d.regular, 1, 3);
    CHECK(cancelTicket(d, 1) == Result::OK); QUEUE_IS(d.regular, 3);
    CHECK(cancelTicket(d, 3) == Result::OK); CHECK(isEmpty(d.regular));

    Visitor v = reg(d, "Neo", VisitorType::REGULAR);     // черга працює після спорожнення
    CHECK(v.id == 8);
    QUEUE_IS(d.regular, 8);
    invariant(d);
    destroyDesk(d);
}

static void test_cancel_twice_and_unknown() {
    DeskState d;
    registerSeven(d);
    CHECK(cancelTicket(d, 3) == Result::OK);
    CHECK(cancelTicket(d, 3) == Result::NotWaiting);     // повторно
    CHECK(cancelTicket(d, 0) == Result::NotWaiting);
    CHECK(cancelTicket(d, -5) == Result::NotWaiting);
    CHECK(cancelTicket(d, 999) == Result::NotWaiting);
    CHECK(d.regular.size == 2 && d.priority.size == 4);
    invariant(d);
    destroyDesk(d);
}

static void test_cancel_current_and_served_rejected() {
    DeskState d;
    registerSeven(d);
    call(d);                                             // 2 біля вікна
    CHECK(cancelTicket(d, 2) == Result::NotWaiting);
    CHECK(d.hasCurrent && d.current.id == 2);            // стан не змінився

    call(d);                                             // 4 біля вікна, 2 обслужений
    CHECK(cancelTicket(d, 2) == Result::NotWaiting);     // обслужений
    CHECK(d.served.size == 1 && servedTopId(d) == 2);
    invariant(d);
    destroyDesk(d);
}

static void test_cancel_does_not_reuse_id_or_touch_streak() {
    DeskState d;
    reg(d, "A", VisitorType::PRIORITY);                  // 1
    reg(d, "B", VisitorType::PRIORITY);                  // 2
    reg(d, "C", VisitorType::REGULAR);                   // 3
    call(d);
    int streak = d.priorityStreak;
    CHECK(cancelTicket(d, 3) == Result::OK);             // скасовано останній виданий
    CHECK(d.priorityStreak == streak);                   // лічильник не змінився
    Visitor v = reg(d, "D", VisitorType::REGULAR);
    CHECK(v.id == 4);                                    // номер 3 не використовується повторно
    invariant(d);
    destroyDesk(d);
}

// ============================================================================
// 10. F06: пошук
// ============================================================================

static void test_find_case_insensitive_partial() {
    DeskState d;
    registerSeven(d);
    std::string text;
    CHECK(findCount(d, "iVaN", &text) == 1);
    CHECK(text.find("Ivan") != std::string::npos);
    CHECK(findCount(d, "an") == 3);                      // Ivan, Oksana, Anna
    CHECK(findCount(d, "a") == 6);                       // усі, крім Petro
    CHECK(findCount(d, "ANNA") == 1);
    destroyDesk(d);
}

static void test_find_no_match() {
    DeskState d;
    registerSeven(d);
    CHECK(findCount(d, "zzz") == 0);
    CHECK(findCount(d, "Ivanovych") == 0);               // підрядок довший за ім'я
    DeskState empty;
    CHECK(findCount(empty, "a") == 0);                   // порожня система
    destroyDesk(empty);
    destroyDesk(d);
    // ПИТАННЯ: findVisitors(d, "") не тестуємо, поки ТЛ не вирішить (усі збіги чи помилка)
}

static void test_find_all_states() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);                                  // 2 Olena обслужена
    call(d);                                             // 4 Maria біля вікна
    CHECK(findCount(d, "Olena") == 1);                   // серед обслужених
    CHECK(findCount(d, "Maria") == 1);                   // поточний
    CHECK(findCount(d, "Petro") == 1);                   // у черзі
    destroyDesk(d);
}

static void test_find_duplicates() {
    DeskState d;
    reg(d, "Ivan", VisitorType::REGULAR);
    reg(d, "Ivan", VisitorType::PRIORITY);
    reg(d, "Ivanna", VisitorType::REGULAR);
    CHECK(findCount(d, "ivan") == 3);                    // усі збіги, однакові імена дозволені
    destroyDesk(d);
}

static void test_find_cancelled_not_found() {
    DeskState d;
    registerSeven(d);
    CHECK(cancelTicket(d, 1) == Result::OK);
    CHECK(findCount(d, "Ivan") == 0);
    destroyDesk(d);
}

static void test_find_special_chars() {
    DeskState d;
    reg(d, "O'Connor", VisitorType::REGULAR);
    reg(d, "Anna-Maria", VisitorType::PRIORITY);
    reg(d, "Ivan Petrenko", VisitorType::REGULAR);
    CHECK(findCount(d, "'C") == 1);
    CHECK(findCount(d, "a-m") == 1);
    CHECK(findCount(d, "n P") == 1);
    CHECK(findCount(d, " ") == 1);
    destroyDesk(d);
}

static void test_find_is_readonly() {
    DeskState d;
    registerSeven(d);
    findCount(d, "a");
    invariant(d);
    QUEUE_IS(d.regular, 1, 3, 7);
    destroyDesk(d);
}

// ============================================================================
// 11. F07: тривалість
// ============================================================================

static void test_record_no_current() {
    DeskState d;
    CHECK(recordDuration(d, 5) == Result::NoCurrent);    // після запуску
    registerSeven(d);
    CHECK(recordDuration(d, 5) == Result::NoCurrent);    // є черги, але нікого біля вікна
    CHECK(d.served.size == 0);
    // ПИТАННЯ: порядок помилок для recordDuration(d, 0) без поточного не тестуємо
    destroyDesk(d);
}

static void test_record_invalid_keeps_current() {
    DeskState d;
    reg(d, "Ivan", VisitorType::PRIORITY);
    call(d);
    CHECK(recordDuration(d, 0) == Result::DurationOutOfRange);
    CHECK(recordDuration(d, -1) == Result::DurationOutOfRange);
    CHECK(recordDuration(d, 241) == Result::DurationOutOfRange);
    CHECK(recordDuration(d, INT_MAX) == Result::DurationOutOfRange);
    CHECK(d.hasCurrent && d.current.id == 1);            // поточний лишився біля вікна
    CHECK(d.served.size == 0);
    CHECK(!d.current.hasDuration);
    invariant(d);
    destroyDesk(d);
}

static void test_record_moves_to_served() {
    DeskState d;
    registerSeven(d);
    call(d);                                             // 2
    CHECK(recordDuration(d, 5) == Result::OK);
    CHECK(!d.hasCurrent);                                // вікно звільнене
    CHECK(d.served.size == 1);
    CHECK(d.served.top && d.served.top->data.id == 2);
    CHECK(d.served.top->data.hasDuration && d.served.top->data.duration == 5);

    CHECK(recordDuration(d, 7) == Result::NoCurrent);    // повторний запис заборонений
    CHECK(d.served.size == 1 && d.served.top->data.duration == 5);
    invariant(d);
    destroyDesk(d);
}

static void test_record_bounds() {
    const int values[] = {1, 240};
    for (int i = 0; i < 2; ++i) {
        DeskState d;
        reg(d, "Ivan", VisitorType::REGULAR);
        call(d);
        CHECK(recordDuration(d, values[i]) == Result::OK);
        CHECK(d.served.top && d.served.top->data.duration == values[i]);
        destroyDesk(d);
    }
}

// ============================================================================
// 12. F08: статистика
// ============================================================================

static void test_stats_empty() {
    DeskState d;
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::NoDurationData);
    CHECK(served == 0 && waiting == 0);                  // served/waiting заповнені завжди
    destroyDesk(d);
}

static void test_stats_average() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);                                  // 2
    call(d); rec(d, 10);                                 // 4
    call(d); rec(d, 6);                                  // 5
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::OK);
    CHECK(served == 3);
    CHECK(waiting == 4);
    CHECK(near(avg, 7.0));
    destroyDesk(d);
}

static void test_stats_current_not_counted() {
    DeskState d;
    registerSeven(d);
    call(d);                                             // 2 біля вікна
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::NoDurationData);
    CHECK(served == 0);                                  // поточний ще не обслужений
    CHECK(waiting == 6);                                 // і не чекає
    destroyDesk(d);
}

static void test_stats_closed_without_duration() {
    DeskState d;
    for (int i = 0; i < 3; ++i) reg(d, "Pr", VisitorType::PRIORITY);
    call(d); call(d); call(d);                           // 1 і 2 закриті без тривалості, 3 біля вікна
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::NoDurationData);
    CHECK(served == 2);                                  // «Обслужено» рахує й тих, хто без тривалості
    CHECK(waiting == 0);
    destroyDesk(d);
}

static void test_stats_average_ignores_closed_without_duration() {
    DeskState d;
    for (int i = 0; i < 3; ++i) reg(d, "Pr", VisitorType::PRIORITY);
    call(d); rec(d, 10);                                 // 1 з тривалістю 10
    call(d);                                             // 2 біля вікна
    call(d);                                             // 2 закрито без тривалості, 3 біля вікна
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::OK);
    CHECK(served == 2);
    CHECK(near(avg, 10.0));                              // середнє лише за записаними
    destroyDesk(d);
    // ПИТАННЯ: округлення середнього на межі .x5 (1,1,1,2 дає 1.25) не тестуємо
}

// ============================================================================
// 13. F09: відкладення
// ============================================================================

static void test_postpone_no_current() {
    DeskState d;
    Visitor out;
    CHECK(postponeCurrent(d, out) == Result::NoCurrent);
    registerSeven(d);
    CHECK(postponeCurrent(d, out) == Result::NoCurrent);
    CHECK(d.regular.size == 3 && d.priority.size == 4);  // нічого не змінилося
    invariant(d);
    destroyDesk(d);
}

static void test_postpone_priority_goes_to_end() {
    DeskState d;
    registerSeven(d);
    call(d);                                             // 2
    Visitor out;
    CHECK(postponeCurrent(d, out) == Result::OK);
    CHECK(out.id == 2);
    CHECK(!out.hasDuration);
    CHECK(!d.hasCurrent);
    QUEUE_IS(d.priority, 4, 5, 6, 2);                    // у кінець своєї черги, номер той самий
    QUEUE_IS(d.regular, 1, 3, 7);
    CHECK(d.served.size == 0);                           // відкладений не став обслуженим
    CHECK(d.priorityStreak == 1);                        // лічильник не змінився
    invariant(d);
    destroyDesk(d);
}

static void test_postpone_regular_goes_to_regular_end() {
    DeskState d;
    registerSeven(d);
    call(d); call(d); call(d);                           // 2, 4, 5
    CHECK(call(d).id == 1);                              // звичайний
    CHECK(d.priorityStreak == 0);
    Visitor out;
    CHECK(postponeCurrent(d, out) == Result::OK);
    CHECK(out.id == 1);
    QUEUE_IS(d.regular, 3, 7, 1);
    QUEUE_IS(d.priority, 6);
    CHECK(d.priorityStreak == 0);                        // неявка звичайного лічильник не чіпає
    invariant(d);
    destroyDesk(d);
}

static void test_postpone_single_visitor() {
    DeskState d;
    reg(d, "Solo", VisitorType::PRIORITY);
    CHECK(call(d).id == 1);
    Visitor out;
    CHECK(postponeCurrent(d, out) == Result::OK);
    QUEUE_IS(d.priority, 1);                             // знову в черзі, і це єдиний
    CHECK(call(d).id == 1);                              // викликається знову
    CHECK(d.priorityStreak == 2);                        // кожен виклик рахується
    invariant(d);
    destroyDesk(d);
}

static void test_postpone_twice() {
    DeskState d;
    registerSeven(d);
    call(d);
    Visitor out;
    CHECK(postponeCurrent(d, out) == Result::OK);
    CHECK(postponeCurrent(d, out) == Result::NoCurrent); // вдруге нікого відкладати
    QUEUE_IS(d.priority, 4, 5, 6, 2);
    invariant(d);
    destroyDesk(d);
}

static void test_postpone_after_record_rejected() {
    DeskState d;
    registerSeven(d);
    call(d);
    rec(d, 5);                                           // вже обслужений
    Visitor out;
    CHECK(postponeCurrent(d, out) == Result::NoCurrent);
    CHECK(d.served.size == 1);
    invariant(d);
    destroyDesk(d);
}

// ============================================================================
// 14. F10: повернення
// ============================================================================

static void test_return_empty_stack() {
    DeskState d;
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::NothingToReturn);
    registerSeven(d);
    CHECK(returnLastServed(d, out) == Result::NothingToReturn);
    destroyDesk(d);
}

static void test_return_priority_to_head() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);                                  // 2 обслужено з тривалістю 5
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(out.id == 2);
    CHECK(!out.hasDuration && out.duration == 0);        // тривалість стерта
    QUEUE_IS(d.priority, 2, 4, 5, 6);                    // перший у черзі свого типу
    CHECK(d.served.size == 0);
    CHECK(d.priority.head && !d.priority.head->data.hasDuration);
    invariant(d);
    destroyDesk(d);
}

static void test_return_regular_to_head() {
    DeskState d;
    registerSeven(d);
    call(d); call(d); call(d);                           // 2, 4, 5
    CHECK(call(d).id == 1);                              // 1 звичайний
    rec(d, 4);                                           // 1 обслужено
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(out.id == 1 && out.type == VisitorType::REGULAR);
    QUEUE_IS(d.regular, 1, 3, 7);
    QUEUE_IS(d.priority, 6);
    invariant(d);
    destroyDesk(d);
}

static void test_return_multiple_reverse_order() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);                                  // 2
    call(d); rec(d, 10);                                 // 4
    call(d); rec(d, 6);                                  // 5
    QUEUE_IS(d.priority, 6);

    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK && out.id == 5);
    CHECK(returnLastServed(d, out) == Result::OK && out.id == 4);
    CHECK(returnLastServed(d, out) == Result::OK && out.id == 2);
    QUEUE_IS(d.priority, 2, 4, 5, 6);
    CHECK(returnLastServed(d, out) == Result::NothingToReturn);
    invariant(d);
    destroyDesk(d);
}

static void test_return_with_current_present() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);                                  // 2 обслужено
    call(d);                                             // 4 біля вікна
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(out.id == 2);
    CHECK(d.hasCurrent && d.current.id == 4);            // поточного не зачіпає
    invariant(d);
    destroyDesk(d);
}

static void test_return_keeps_streak() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);
    call(d); rec(d, 5);
    int streak = d.priorityStreak;
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(d.priorityStreak == streak);
    destroyDesk(d);
}

static void test_return_updates_stats() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);                                  // 2
    call(d); rec(d, 10);                                 // 4
    call(d); rec(d, 6);                                  // 5
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK && out.id == 5);
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::OK);
    CHECK(served == 2);                                  // «Обслужено» зменшилось на 1
    CHECK(waiting == 5);
    CHECK(near(avg, 7.5));                               // (5 + 10) / 2
    destroyDesk(d);
}

static void test_return_only_served_then_no_data() {
    DeskState d;
    reg(d, "Solo", VisitorType::REGULAR);
    call(d); rec(d, 5);
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::OK && near(avg, 5.0));

    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(getStats(d, served, waiting, avg) == Result::NoDurationData);
    CHECK(served == 0 && waiting == 1);
    destroyDesk(d);
}

static void test_return_then_call_again() {
    DeskState d;
    reg(d, "Solo", VisitorType::REGULAR);
    call(d); rec(d, 5);
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(call(d).id == 1);                              // повернений викликається першим
    rec(d, 9);
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::OK);
    CHECK(served == 1 && near(avg, 9.0));                // нова тривалість записана заново
    invariant(d);
    destroyDesk(d);
}

static void test_return_is_first_called() {
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);                                  // 2
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(call(d).id == 2);                              // стоїть першим у пільговій черзі
    invariant(d);
    destroyDesk(d);
}

// ============================================================================
// 15. Контрольний сценарій таблиці 7 повністю
// ============================================================================

static void test_table7_average_rows() {
    // Викликати 2, 4, 5 із тривалостями 5, 10, 6: середнє 7.0
    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);
    call(d); rec(d, 10);
    call(d); rec(d, 6);
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::OK && near(avg, 7.0));

    // Повернути талон 5: перший у PRIORITY (перед 6), середнє 7.5
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK && out.id == 5);
    QUEUE_IS(d.priority, 5, 6);
    CHECK(getStats(d, served, waiting, avg) == Result::OK && near(avg, 7.5));
    destroyDesk(d);
}

static void test_table7_cancel_row() {
    // Скасувати талон 5 до викликів: порядок 2, 4, 6, 1, 3, 7
    DeskState d;
    registerSeven(d);
    CHECK(cancelTicket(d, 5) == Result::OK);
    const int expect[] = {2, 4, 6, 1, 3, 7};
    for (int i = 0; i < 6; ++i) CHECK(call(d).id == expect[i]);
    destroyDesk(d);
}

static void test_table7_return_after_all_called() {
    // Викликати всіх сімох, повернути останнього обслуженого: це талон 3, а 7 лишається біля вікна
    DeskState d;
    registerSeven(d);
    for (int i = 0; i < 7; ++i) call(d);
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK);
    CHECK(out.id == 3);
    QUEUE_IS(d.regular, 3);
    CHECK(d.hasCurrent && d.current.id == 7);
    invariant(d);
    destroyDesk(d);
}

// ============================================================================
// 16. Повний цикл і пам'ять
// ============================================================================

static void test_full_cycle() {
    DeskState d;
    Visitor a = reg(d, "Ivan", VisitorType::PRIORITY);
    Visitor b = reg(d, "Olena", VisitorType::REGULAR);
    CHECK(call(d).id == a.id);
    rec(d, 3);
    CHECK(call(d).id == b.id);
    Visitor p;
    CHECK(postponeCurrent(d, p) == Result::OK);
    CHECK(call(d).id == b.id);
    rec(d, 8);
    Visitor out;
    CHECK(returnLastServed(d, out) == Result::OK && out.id == b.id);
    CHECK(call(d).id == b.id);
    rec(d, 4);
    int served = -1, waiting = -1;
    double avg = -1;
    CHECK(getStats(d, served, waiting, avg) == Result::OK);
    CHECK(served == 2 && waiting == 0 && near(avg, 3.5));
    invariant(d);
    destroyDesk(d);
}

static void test_destroy_empty_and_twice() {
    DeskState empty;
    destroyDesk(empty);                                  // порожній стан

    DeskState d;
    registerSeven(d);
    call(d); rec(d, 5);
    call(d);
    destroyDesk(d);
    CHECK(d.regular.size == 0 && d.priority.size == 0 && d.served.size == 0);
    CHECK(d.regular.head == nullptr && d.priority.head == nullptr && d.served.top == nullptr);
    destroyDesk(d);                                      // повторно: не має падати (ASan)
}

static void test_destroy_with_everything() {
    DeskState d;
    for (int i = 0; i < 200; ++i) reg(d, "Vi", (i % 2) ? VisitorType::REGULAR : VisitorType::PRIORITY);
    for (int i = 0; i < 50; ++i) call(d);
    rec(d, 5);
    destroyDesk(d);                                      // AddressSanitizer не має показати витоків
}

// ============================================================================

int main() {
    RUN(test_queue_order);
    RUN(test_queue_remove_head_middle_tail);
    RUN(test_queue_single_element);
    RUN(test_queue_clear);
    RUN(test_queue_many);

    RUN(test_stack_lifo);
    RUN(test_stack_clear);
    RUN(test_stack_many);

    RUN(test_visitor_output);

    RUN(test_validate_name_ok);
    RUN(test_validate_name_empty);
    RUN(test_validate_name_too_long);
    RUN(test_validate_name_bad_chars);
    RUN(test_validate_duration);

    RUN(test_register_ids_and_queues);
    RUN(test_register_trims_name);
    RUN(test_register_invalid_does_not_burn_id);
    RUN(test_register_many);

    RUN(test_show_queues);

    RUN(test_call_order_table7);
    RUN(test_call_empty_queues);
    RUN(test_call_first_call_pushes_nothing);
    RUN(test_call_empty_with_current_keeps_everything);
    RUN(test_call_streak_counter);
    RUN(test_call_priority_empty_resets_counter);
    RUN(test_call_streak_continues_without_regular);
    RUN(test_call_postponed_counts_as_call);
    RUN(test_call_stress);

    RUN(test_ahead_table7);
    RUN(test_ahead_after_cancel);
    RUN(test_ahead_not_waiting);
    RUN(test_ahead_current_not_counted);
    RUN(test_ahead_respects_streak);
    RUN(test_ahead_simple_cases);
    RUN(test_ahead_is_readonly);
    RUN(test_ahead_after_return);

    RUN(test_cancel_head_middle_tail);
    RUN(test_cancel_twice_and_unknown);
    RUN(test_cancel_current_and_served_rejected);
    RUN(test_cancel_does_not_reuse_id_or_touch_streak);

    RUN(test_find_case_insensitive_partial);
    RUN(test_find_no_match);
    RUN(test_find_all_states);
    RUN(test_find_duplicates);
    RUN(test_find_cancelled_not_found);
    RUN(test_find_special_chars);
    RUN(test_find_is_readonly);

    RUN(test_record_no_current);
    RUN(test_record_invalid_keeps_current);
    RUN(test_record_moves_to_served);
    RUN(test_record_bounds);

    RUN(test_stats_empty);
    RUN(test_stats_average);
    RUN(test_stats_current_not_counted);
    RUN(test_stats_closed_without_duration);
    RUN(test_stats_average_ignores_closed_without_duration);

    RUN(test_postpone_no_current);
    RUN(test_postpone_priority_goes_to_end);
    RUN(test_postpone_regular_goes_to_regular_end);
    RUN(test_postpone_single_visitor);
    RUN(test_postpone_twice);
    RUN(test_postpone_after_record_rejected);

    RUN(test_return_empty_stack);
    RUN(test_return_priority_to_head);
    RUN(test_return_regular_to_head);
    RUN(test_return_multiple_reverse_order);
    RUN(test_return_with_current_present);
    RUN(test_return_keeps_streak);
    RUN(test_return_updates_stats);
    RUN(test_return_only_served_then_no_data);
    RUN(test_return_then_call_again);
    RUN(test_return_is_first_called);

    RUN(test_table7_average_rows);
    RUN(test_table7_cancel_row);
    RUN(test_table7_return_after_all_called);

    RUN(test_full_cycle);
    RUN(test_destroy_empty_and_twice);
    RUN(test_destroy_with_everything);

    std::cout << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed ? 1 : 0;
}
