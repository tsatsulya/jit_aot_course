#pragma once

#include <vector>
#include <string>
#include "context.h"

class BasicBlock;
class NameContext;

enum class InstrKind { Add, Mul, Sub, Jump, CondJump, Return, Param, Cmp, Phi };

class Value {
public:
    virtual std::string str(NameContext& ctx) const = 0;
    virtual ~Value() = default;
};

class Parameter : public Value {
    std::string name;

public:
    Parameter(const std::string& n) : name(n) {}
    std::string str(NameContext& ctx) const override {
        return ctx.getValueName(this);
    }
    std::string getName() {return name; }
};

class Instruction : public Value {
protected:
    InstrKind kind;
    std::vector<Value*> operands;

public:
    Instruction(InstrKind k, const std::vector<Value*>& ops) : kind(k), operands(ops) {}
    virtual InstrKind getKind() const { return kind; }
    virtual const std::vector<Value*>& getOperands() const { return operands; }
};