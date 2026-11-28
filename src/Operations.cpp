#include "Operations.h"

// Module B: F03 callNext, F04 peopleAhead, F09 postponeCurrent, F10 returnLastServed.
// The 3:1 counter changes only inside callNext.

namespace {

LinkedQueue& queueFor(DeskState& d, VisitorType type) {
    if (type == VisitorType::PRIORITY) {
        return d.priority;
    }
    return d.regular;
}

// People who would be called before `id`, using the same rule as callNext.
// Returns -1 when `id` is not waiting. Does not touch the desk.
int countAhead(const QNode* priority, const QNode* regular, int streak, int id) {
    const bool regularTurn = (streak >= 3 && regular != nullptr) || priority == nullptr;
    if (regularTurn) {
        if (regular == nullptr) {
            return -1;
        }
        if (regular->data.id == id) {
            return 0;
        }
        const int rest = countAhead(priority, regular->next, 0, id);
        if (rest < 0) {
            return -1;
        }
        return rest + 1;
    }

    if (priority->data.id == id) {
        return 0;
    }
    const int rest = countAhead(priority->next, regular, streak + 1, id);
    if (rest < 0) {
        return -1;
    }
    return rest + 1;
}

}  // namespace

Result callNext(DeskState& d, Visitor& out) {
    if (isEmpty(d.priority) && isEmpty(d.regular)) {
        return Result::QueuesEmpty;
    }

    // At most three PRIORITY calls in a row while REGULAR still has someone.
    // Any REGULAR call, including one taken because PRIORITY is empty, resets the counter.
    const bool regularTurn =
        (d.priorityStreak >= 3 && !isEmpty(d.regular)) || isEmpty(d.priority);

    LinkedQueue& source = regularTurn ? d.regular : d.priority;
    Visitor next;
    if (!popFront(source, next)) {
        return Result::QueuesEmpty;
    }

    if (d.hasCurrent) {
        push(d.served, d.current);
        d.current = Visitor();
        d.hasCurrent = false;
    }

    if (regularTurn) {
        d.priorityStreak = 0;
    } else {
        ++d.priorityStreak;
    }

    d.current = next;
    d.hasCurrent = true;
    out = next;
    return Result::OK;
}

Result peopleAhead(const DeskState& d, int id, int& ahead) {
    const int n = countAhead(d.priority.head, d.regular.head, d.priorityStreak, id);
    if (n < 0) {
        return Result::NotWaiting;
    }
    ahead = n;
    return Result::OK;
}

Result postponeCurrent(DeskState& d, Visitor& out) {
    if (!d.hasCurrent) {
        return Result::NoCurrent;
    }

    Visitor back = d.current;
    back.hasDuration = false;
    back.duration = 0;
    pushBack(queueFor(d, back.type), back);

    d.current = Visitor();
    d.hasCurrent = false;
    out = back;
    return Result::OK;
}

Result returnLastServed(DeskState& d, Visitor& out) {
    if (isEmpty(d.served)) {
        return Result::NothingToReturn;
    }

    Visitor back;
    if (!pop(d.served, back)) {
        return Result::NothingToReturn;
    }

    back.hasDuration = false;
    back.duration = 0;
    pushFront(queueFor(d, back.type), back);
    out = back;
    return Result::OK;
}
