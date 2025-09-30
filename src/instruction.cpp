#include "instruction.h"
#include "context.h"

std::string Parameter::str(NameContext& /*ctx*/) const {
    return "%" + name;
}

std::string const Parameter::getName() const {
    return "%" + name;
}