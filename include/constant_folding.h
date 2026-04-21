#pragma once

#include "instruction.h"
#include "basic_block.h"
#include "bin_ops.h"
#include "function.h"
#include <memory>
#include <optional>

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
                    std::optional<int> folded = tryFoldBinaryOp(binaryOp);
                    if (folded.has_value()) {
                        auto foldedInstr = std::make_unique<ConstantInstruction>(*folded);
                        ConstantInstruction* replacement = foldedInstr.get();
                        replaceAllUsesWith(binaryOp, replacement, bb);

                        binaryOp->dropAllOperands();
                        *it = std::move(foldedInstr);
                        changed = true;
                    }
                }
            }

            ++it;
        }

        return changed;
    }

private:
    static std::optional<int> tryFoldBinaryOp(BinaryOp* op) {
        auto lhs = dynamic_cast<Constant*>(op->getOperands()[0]);
        auto rhs = dynamic_cast<Constant*>(op->getOperands()[1]);

        if (!lhs || !rhs) {
            return std::nullopt;
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
                    return std::nullopt;
                }
                result = static_cast<unsigned int>(lhsVal) >> rhsVal;
                break;
            case InstrKind::Shl:
                if (rhsVal < 0 || rhsVal >= 32) {
                    return std::nullopt;
                }
                result = lhsVal << rhsVal;
                break;
            case InstrKind::And:
                result = lhsVal & rhsVal;
                break;
            default:
                return std::nullopt;
        }

        return result;
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
                instr->replaceOperand(oldInstr, newVal);
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
