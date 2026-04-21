#pragma once

#include <vector>
#include <string>
#include <memory>

class BasicBlock;
class NameContext;
class Instruction;

enum class InstrKind {
    Add,
    Mul,
    Sub,
    Jump,
    CondJump,
    Return,
    Param,
    Cmp,
    Phi,
    Shr,
    And,
    Shl,
    Call,
    Constant,
    NullCheck,
    BoundsCheck
};

class Value {
    std::vector<Instruction*> users;

public:
    enum class ValueKind { Parameter, Constant, Instruction };
    virtual std::string str(NameContext& ctx) const = 0;
    virtual ValueKind getValueKind() const = 0;
    void addUser(Instruction* user);
    void removeUser(Instruction* user);
    const std::vector<Instruction*>& getUsers() const;
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
    void addOperand(Value* operand);

public:
    Instruction(InstrKind k, const std::vector<Value*>& ops);
    virtual InstrKind getKind() const;
    virtual const std::vector<Value*>& getOperands() const;
    virtual std::vector<Value*>& getOperands();
    void replaceOperand(Value* oldValue, Value* newValue);
    void dropAllOperands();
    virtual ~Instruction();
    ValueKind getValueKind() const override;

    virtual void updateCFG() = 0;
    virtual void print(std::ostream& os) const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Instruction& instr) {
        instr.print(os);
        return os;
    }
};

class ConstantInstruction : public Instruction {
    std::unique_ptr<Constant> constant;

public:
    ConstantInstruction(int value, const std::string& name)
        : Instruction(InstrKind::Constant, {}), constant(std::make_unique<Constant>(value, name)) {}

    explicit ConstantInstruction(int value) : ConstantInstruction(value, std::to_string(value)) {}

    const Constant* getConstant() const { return constant.get(); }
    Constant* getConstant() { return constant.get(); }

    std::string str(NameContext& ctx) const override;
    void updateCFG() override {}
    void print(std::ostream& os) const override;
};
