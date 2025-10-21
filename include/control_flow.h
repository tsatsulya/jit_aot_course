#pragma once

#include "instruction.h"
#include "basic_block.h"
#include "context.h"

class Return : public Instruction {
    Value* retVal;

public:
    Return(Value* val = nullptr) : Instruction(InstrKind::Return, val ? std::vector<Value*>{val} : std::vector<Value*>{}), retVal(val) {}

    std::string str(NameContext& ctx) const override {
        return retVal ? "return " + ctx.getValueName(retVal) : "return";
    }

    void updateCFG() override {
        // Return terminates the block, but CFG edges are handled by the function
    }

    void print(std::ostream& os) const override {
        if (retVal) {
            os << "return " << retVal;
        } else {
            os << "return";
        }
    }
};

class Jump : public Instruction {
private:
    std::string targetName;
    BasicBlock* targetBlock;
    BasicBlock* parentBlock;

public:
    Jump(const std::string& target)
        : Instruction(InstrKind::Jump, {}), targetName(target),
          targetBlock(nullptr), parentBlock(nullptr) {}

    void setParentBlock(BasicBlock* parent) {
        parentBlock = parent;
    }

    void setTargetBlock(BasicBlock* target) {
        targetBlock = target;
    }

    const std::string& getTargetName() const {
        return targetName;
    }

    BasicBlock* getTarget() const {
        return targetBlock;
    }

    void updateCFG() override {
        if (!parentBlock || !targetBlock) {
            return;
        }

        parentBlock->clearSuccessors();
        parentBlock->addSuccessor(targetBlock);
        targetBlock->addPredecessor(parentBlock);
    }

    void print(std::ostream& os) const override {
        os << "jump " << targetName;
    }

    std::string str(NameContext& /*ctx*/) const override {
        return "jump " + targetName;
    }
};

class CondJump : public Instruction {
private:
    std::string condition;
    std::string trueTargetName;
    std::string falseTargetName;
    BasicBlock* trueTargetBlock;
    BasicBlock* falseTargetBlock;
    BasicBlock* parentBlock;

public:
    CondJump(const std::string& cond, const std::string& trueTarget, const std::string& falseTarget)
        : Instruction(InstrKind::CondJump, {}),
          condition(cond), trueTargetName(trueTarget), falseTargetName(falseTarget),
          trueTargetBlock(nullptr), falseTargetBlock(nullptr), parentBlock(nullptr) {}

    void setParentBlock(BasicBlock* parent) {
        parentBlock = parent;
    }

    void setTrueTargetBlock(BasicBlock* target) {
        trueTargetBlock = target;
    }

    void setFalseTargetBlock(BasicBlock* target) {
        falseTargetBlock = target;
    }

    const std::string& getCondition() const { return condition; }
    const std::string& getTrueTargetName() const { return trueTargetName; }
    const std::string& getFalseTargetName() const { return falseTargetName; }

    BasicBlock* getTrueTarget() const {
        return trueTargetBlock;
    }

    BasicBlock* getFalseTarget() const {
        return falseTargetBlock;
    }

    void updateCFG() override {
        if (!parentBlock) return;

        parentBlock->clearSuccessors();

        if (trueTargetBlock) {
            parentBlock->addSuccessor(trueTargetBlock);
            trueTargetBlock->addPredecessor(parentBlock);
        }

        if (falseTargetBlock) {
            parentBlock->addSuccessor(falseTargetBlock);
            falseTargetBlock->addPredecessor(parentBlock);
        }
    }

    void print(std::ostream& os) const override {
        os << "if (" << condition << ") ( jump " << trueTargetName
           << " ) else ( jump " << falseTargetName << " )\n";
    }

    std::string str(NameContext& /*ctx*/) const override {
        return "if (" + condition + ") ( jump " + trueTargetName
           + " ) else ( jump " + falseTargetName + " )";
    }
};


class Phi : public Instruction {
    std::vector<std::pair<BasicBlock*, Value*>> incoming;

public:
    Phi() : Instruction(InstrKind::Phi, {}) {}

    void addIncoming(BasicBlock* pred, Value* val) {
        incoming.emplace_back(pred, val);
        operands.push_back(val);
    }

    std::string str(NameContext& ctx) const override {
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

    void updateCFG() override {
    }

    void print(std::ostream& os) const override {
        os << "phi ";
        bool first = true;
        for (const auto& [pred, val] : incoming) {
            if (!first) os << ", ";
            os << "[" << (val ? "value" : "null") << ", " << (pred ? pred->getName() : "null") << "]";
            first = false;
        }
    }

    const std::vector<std::pair<BasicBlock*, Value*>>& getIncoming() const { return incoming; }
};