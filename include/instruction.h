#pragma once

#include <vector>
#include <string>

class BasicBlock;
class NameContext;

enum class InstrKind { Add, Mul, Sub, Jump, CondJump, Return, Param, Cmp, Phi };

class Value {
public:
    enum class ValueKind { Parameter, Constant, Instruction };
    virtual std::string str(NameContext& ctx) const = 0;
    virtual ValueKind getValueKind() const = 0;
    virtual ~Value() = default;
};

class Parameter : public Value {
    std::string name;

public:
    Parameter(const std::string& n) : name(n) {}
    std::string str(NameContext& /*ctx*/) const override;
    std::string const getName() const;
    virtual ~Parameter() = default;
    ValueKind getValueKind() const override {
        return ValueKind::Parameter;
    }
};

class Constant : public Value {
    int value;
    std::string name;

public:
    Constant(int v, const std::string& n) : value(v), name(n) {}

    ValueKind getValueKind() const override {
        return ValueKind::Constant;
    }

    std::string str(NameContext& /*ctx*/) const override {
        return name; 
    }
    int getValue() const { return value; }
    std::string getName() const { return name; }
};

class Instruction : public Value {
protected:
    InstrKind kind;
    std::vector<Value*> operands;

public:
    Instruction(InstrKind k, const std::vector<Value*>& ops) : kind(k), operands(ops) {}
    virtual InstrKind getKind() const { return kind; }
    virtual const std::vector<Value*>& getOperands() const { return operands; }
    virtual ~Instruction() = default;
    ValueKind getValueKind() const override {
        return ValueKind::Instruction;
    }
};