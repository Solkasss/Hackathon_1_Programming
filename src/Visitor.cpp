#include "Visitor.h"

std::string typeToStr(VisitorType v) {
    if (v == VisitorType::REGULAR) {
        return "REGULAR";
    } else {
        return "PRIORITY";
    }
}

std::ostream& operator<<(std::ostream& os, const Visitor& v) {
    return os << "#" << v.id << " " << v.name << " (" << typeToStr(v.type) << ")"; // #123 Oleg (REGULAR)
}