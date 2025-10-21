#pragma once

#include "instruction.h"
#include "context.h"

enum class CmpOp {
    Eq, Ne, Lt, Le, Gt, Ge
};

class BinaryOp : public Instruction {
public:
    BinaryOp(InstrKind kind, Value* lhs, Value* rhs) : Instruction(kind, {lhs, rhs}) {}

    std::string str(NameContext& ctx) const override {
        std::string op;
        switch (kind) {
            case InstrKind::Add: op = "add"; break;
            case InstrKind::Mul: op = "mul"; break;
            case InstrKind::Sub: op = "sub"; break;
            default: op = "unknown"; break;
        }
        return ctx.getValueName(this) + " = " + op + " " + ctx.getValueName(operands[0]) + ", " + ctx.getValueName(operands[1]);
    }

    void updateCFG() override {
        // Binary operations don't affect CFG
    }

    void print(std::ostream& os) const override {
        std::string op;
        switch (kind) {
            case InstrKind::Add: op = "add"; break;
            case InstrKind::Mul: op = "mul"; break;
            case InstrKind::Sub: op = "sub"; break;
            default: op = "unknown"; break;
        }
        os << op << " " << operands[0] << ", " << operands[1];
    }
};

class Cmp : public Instruction {
    CmpOp cmpOp;

public:
    Cmp(CmpOp op, Value* lhs, Value* rhs) : Instruction(InstrKind::Cmp, {lhs, rhs}), cmpOp(op) {}

    std::string str(NameContext& ctx) const override {
        const char* names[] = {"eq","ne","lt","le","gt","ge"};
        return ctx.getValueName(this) + " = cmp." + names[(int)cmpOp] + " " + ctx.getValueName(operands[0]) + ", " + ctx.getValueName(operands[1]);
    }

    void updateCFG() override {
        // Cmp operations don't afsfect CFG directly
    }

    void print(std::ostream& os) const override {
        const char* names[] = {"eq","ne","lt","le","gt","ge"};
        os << "cmp." << names[(int)cmpOp] << " " << operands[0] << ", " << operands[1];
    }
};