#pragma once

#include "instruction.h"
#include "basic_block.h"
#include <unordered_map>
#include "context.h"
#include "program.h"

class Return : public Instruction {
    Value* retVal;

public:
    Return(Value* val = nullptr) : Instruction(InstrKind::Return, val ? std::vector<Value*>{val} : std::vector<Value*>{}), retVal(val) {}

    std::string str(NameContext& ctx) const override {
        return retVal ? "return " + ctx.getValueName(retVal) : "return";
    }
};

class Jump : public Instruction {
    BasicBlock* target;

public:
    Jump(BasicBlock* tgt) : Instruction(InstrKind::Jump, {}), target(tgt) {}

    std::string str(NameContext& /*ctx*/) const override {
        return "jump " + target->getName();
    };

    BasicBlock* getTarget() const { return target; }
};

class CondJump : public Instruction {
    BasicBlock* trueTarget;
    BasicBlock* falseTarget;

public:
    CondJump(Value* cond, BasicBlock* t, BasicBlock* f) : Instruction(InstrKind::CondJump, {cond}), trueTarget(t), falseTarget(f) {}

    std::string str(NameContext& ctx) const override {
        return "cjump " + ctx.getValueName(operands[0]) + ", " + trueTarget->getName() + ", " + falseTarget->getName();
    }

    BasicBlock* getTrueTarget() const { return trueTarget; }
    BasicBlock* getFalseTarget() const { return falseTarget; }
};

class Phi : public Instruction {
    std::vector<std::pair<BasicBlock*, Value*>> incoming;

public:
    Phi() : Instruction(InstrKind::Phi, {}) {}

    void addIncoming(BasicBlock* pred, Value* val) {
        incoming.emplace_back(pred, val);
        operands.push_back(val);
    }

    std::string str(NameContext& ctx) const {
        std::string s = ctx.getValueName(this) + " = phi ";
        bool first = true;
        for (const auto& [pred, val] : incoming) {
            if (!first) s += ", ";
            if (!pred || !val) {
                s += "[INVALID]";
            } else {
                s += "[" + ctx.getValueName(val) + ", " + pred->getName() + "]";
            }
            first = false;
        }
        return s;
    }
};