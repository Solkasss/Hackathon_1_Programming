#include "Visitor.h"

#include <iostream>
#include <sstream>

bool checkOutput(const Visitor& visitor, const std::string& expected) {
    std::ostringstream output;
    output << visitor;
    if (output.str() == expected) {
        return true;
    }

    std::cerr << "Expected: " << expected << '\n'
              << "Actual:   " << output.str() << '\n';
    return false;
}

int main() {
    const Visitor regular{1, "Ivan Petrenko", VisitorType::REGULAR};
    const Visitor priority{2, "Anna-Maria", VisitorType::PRIORITY};

    const bool regularPassed = checkOutput(regular, "#1 Ivan Petrenko (REGULAR)");
    const bool priorityPassed = checkOutput(priority, "#2 Anna-Maria (PRIORITY)");
    return regularPassed && priorityPassed ? 0 : 1;
}
