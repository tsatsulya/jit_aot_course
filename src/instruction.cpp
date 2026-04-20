#include "instruction.h"

Parameter::Parameter(const std::string& n) : name(n) {}

std::string Parameter::str(NameContext& /*ctx*/) const {
    return name;
}

std::string Parameter::getName() const {
    return name;
}

Value::ValueKind Parameter::getValueKind() const {
    return Value::ValueKind::Parameter;
}

Constant::Constant(int v, const std::string& n) : value(v), name(n) {}

Value::ValueKind Constant::getValueKind() const {
    return Value::ValueKind::Constant;
}

std::string Constant::str(NameContext& /*ctx*/) const {
    return name;
}

int Constant::getValue() const { return value; }

std::string Constant::getName() const { return name; }

Instruction::Instruction(InstrKind k, const std::vector<Value*>& ops) : kind(k), operands(ops) {}

InstrKind Instruction::getKind() const { return kind; }

const std::vector<Value*>& Instruction::getOperands() const { return operands; }

std::vector<Value*>& Instruction::getOperands() { return operands; }

void Instruction::replaceOperand(Value* oldValue, Value* newValue) {
    for (auto& operand : operands) {
        if (operand == oldValue) {
            operand = newValue;
        }
    }
}

Value::ValueKind Instruction::getValueKind() const {
    return Value::ValueKind::Instruction;
}
