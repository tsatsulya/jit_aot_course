#include "context.h"
#include "instruction.h"

std::string NameContext::getValueName(const Value* val) {
    auto it = valueNames.find(val);
    if (it != valueNames.end()) return it->second;

    std::string name;
    switch (val->getValueKind()) {
        case Value::ValueKind::Parameter:
            name = static_cast<const Parameter*>(val)->str(*this);
            break;
        case Value::ValueKind::Constant:
            name = static_cast<const Constant*>(val)->str(*this);
            break;
        case Value::ValueKind::Instruction:
            name = "%v" + std::to_string(nextId++);
            break;
    }

    valueNames[val] = name;
    return name;
}