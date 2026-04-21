#include "instruction.h"
#include "context.h"
#include <algorithm>
#include <iostream>

void Value::addUser(Instruction* user) {
    if (!user) {
        return;
    }
    if (std::find(users.begin(), users.end(), user) == users.end()) {
        users.push_back(user);
    }
}

void Value::removeUser(Instruction* user) {
    users.erase(std::remove(users.begin(), users.end(), user), users.end());
}

const std::vector<Instruction*>& Value::getUsers() const {
    return users;
}

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

Instruction::Instruction(InstrKind k, const std::vector<Value*>& ops) : kind(k) {
    for (Value* operand : ops) {
        addOperand(operand);
    }
}

Instruction::~Instruction() = default;

InstrKind Instruction::getKind() const { return kind; }

const std::vector<Value*>& Instruction::getOperands() const { return operands; }

std::vector<Value*>& Instruction::getOperands() { return operands; }

void Instruction::addOperand(Value* operand) {
    operands.push_back(operand);
    if (operand) {
        operand->addUser(this);
    }
}

void Instruction::replaceOperand(Value* oldValue, Value* newValue) {
    for (auto& operand : operands) {
        if (operand == oldValue) {
            if (oldValue) {
                oldValue->removeUser(this);
            }
            operand = newValue;
            if (newValue) {
                newValue->addUser(this);
            }
        }
    }
}

void Instruction::dropAllOperands() {
    for (Value* operand : operands) {
        if (operand) {
            operand->removeUser(this);
        }
    }
    operands.clear();
}

Value::ValueKind Instruction::getValueKind() const {
    return Value::ValueKind::Instruction;
}

std::string ConstantInstruction::str(NameContext& ctx) const {
    return ctx.getValueName(this) + " = const " + constant->str(ctx);
}

void ConstantInstruction::print(std::ostream& os) const {
    os << "const " << constant->getName();
}
