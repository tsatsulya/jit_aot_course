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
    Parameter(const std::string& n);
    std::string str(NameContext& ctx) const override;
    std::string getName() const;
    virtual ~Parameter() = default;
    ValueKind getValueKind() const override;
};

class Constant : public Value {
    int value;
    std::string name;

public:
    Constant(int v, const std::string& n);
    ValueKind getValueKind() const override;
    std::string str(NameContext& ctx) const override;
    int getValue() const;
    std::string getName() const;
};

class Instruction : public Value {
protected:
    InstrKind kind;
    std::vector<Value*> operands;

public:
    Instruction(InstrKind k, const std::vector<Value*>& ops);
    virtual InstrKind getKind() const;
    virtual const std::vector<Value*>& getOperands() const;
    virtual ~Instruction() = default;
    ValueKind getValueKind() const override;

    virtual void updateCFG() = 0;
    virtual void print(std::ostream& os) const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Instruction& instr) {
        instr.print(os);
        return os;
    }
};