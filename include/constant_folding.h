#pragma once

#include "instruction.h"
#include "basic_block.h"
#include "bin_ops.h"
#include <memory>

class ConstantFolding {
public:
    static bool foldConstants(BasicBlock& bb) {
        bool changed = false;

        auto& instructions = bb.getInstructions();

        for (auto it = instructions.begin(); it != instructions.end(); ) {
            auto& instr = *it;

            if (auto binaryOp = dynamic_cast<BinaryOp*>(instr.get())) {
                if (binaryOp->getKind() == InstrKind::Mul ||
                    binaryOp->getKind() == InstrKind::Shr ||
                    binaryOp->getKind() == InstrKind::Shl ||
                    binaryOp->getKind() == InstrKind::And) {
                    Value* folded = tryFoldBinaryOp(binaryOp);
                    if (folded) {
                        replaceAllUsesWith(binaryOp, folded, bb);

                        it = instructions.erase(it);
                        changed = true;
                        continue;
                    }
                }
            }

            ++it;
        }

        return changed;
    }

private:
    static Value* tryFoldBinaryOp(BinaryOp* op) {
        auto lhs = dynamic_cast<Constant*>(op->getOperands()[0]);
        auto rhs = dynamic_cast<Constant*>(op->getOperands()[1]);

        if (!lhs || !rhs) {
            return nullptr;
        }

        int lhsVal = lhs->getValue();
        int rhsVal = rhs->getValue();
        int result;

        switch (op->getKind()) {
            case InstrKind::Mul:
                result = lhsVal * rhsVal;
                break;
            case InstrKind::Shr:
                if (rhsVal < 0 || rhsVal >= 32) {
                    return nullptr;
                }
                result = static_cast<unsigned int>(lhsVal) >> rhsVal;
                break;
            case InstrKind::Shl:
                if (rhsVal < 0 || rhsVal >= 32) {
                    return nullptr;
                }
                result = lhsVal << rhsVal;
                break;
            case InstrKind::And:
                result = lhsVal & rhsVal;
                break;
            default:
                return nullptr;
        }

        return new Constant(result, std::to_string(result));
    }

    static void replaceAllUsesWith(Instruction* oldInstr, Value* newVal, BasicBlock& bb) {
        auto& instructions = bb.getInstructions();

        bool foundOld = false;
        for (auto& instr : instructions) {
            if (instr.get() == oldInstr) {
                foundOld = true;
                continue;
            }

            if (foundOld) {
                auto& operands = const_cast<std::vector<Value*>&>(instr->getOperands());
                for (auto& operand : operands) {
                    if (operand == oldInstr) {
                        operand = newVal;
                    }
                }
            }
        }
    }
};

class ConstantFoldingPass {
public:
    static bool runOnFunction(Function& function) {
        bool changed = false;

        for (auto& bb : function.getBasicBlocks()) {
            if (ConstantFolding::foldConstants(*bb)) {
                changed = true;
            }
        }

        return changed;
    }
};