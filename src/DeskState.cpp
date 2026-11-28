#include "DeskState.h"

void destroyDesk(DeskState& d) {
    clear(d.regular);
    clear(d.priority);
    clear(d.served);
    d.current = Visitor();
    d.hasCurrent = false;
    d.priorityStreak = 0;
}
