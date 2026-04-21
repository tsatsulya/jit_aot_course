#pragma once

#include "instruction.h"
#include "basic_block.h"
#include "bin_ops.h"
#include <cstddef>
#include <memory>
#include <functional>

class PeepholeOptimizer {
public:
    static bool optimizeBlock(BasicBlock& bb) {
        bool changed = false;

        for (size_t i = 0; i < bb.getInstructions().size(); ++i) {
            auto& instr = bb.getInstructions()[i];

            if (auto binaryOp = dynamic_cast<BinaryOp*>(instr.get())) {
                if (applyMulPeepholes(binaryOp, bb, i) ||
                    applyAndPeepholes(binaryOp, bb, i) ||
                    applyShrPeepholes(binaryOp, bb, i)) {
                    changed = true;
                    i = static_cast<size_t>(-1);
                    continue;
                }
            }
        }

        return changed;
    }

private:
    // Mul Peephole 1: Multiply by 0 -> 0
    // Mul Peephole 2: Multiply by 1 -> identity
    // Mul Peephole 3: Multiply by power of 2 -> shift left
    static bool applyMulPeepholes(BinaryOp* mul, BasicBlock& bb, size_t index) {
        if (mul->getKind() != InstrKind::Mul) return false;

        auto lhs = dynamic_cast<Constant*>(mul->getOperands()[0]);
        auto rhs = dynamic_cast<Constant*>(mul->getOperands()[1]);

        if (lhs && lhs->getValue() == 0) {
            replaceWithConstant(mul, 0, bb, index);
            return true;
        }
        if (rhs && rhs->getValue() == 0) {
            replaceWithConstant(mul, 0, bb, index);
            return true;
        }

        if (lhs && lhs->getValue() == 1) {
            replaceWithOperand(mul, mul->getOperands()[1], bb, index);
            return true;
        }
        if (rhs && rhs->getValue() == 1) {
            replaceWithOperand(mul, mul->getOperands()[0], bb, index);
            return true;
        }

        if (rhs && isPowerOfTwo(rhs->getValue())) {
            int shiftAmount = log2(rhs->getValue());
            auto shiftConst = new Constant(shiftAmount, std::to_string(shiftAmount));
            auto shlOp = new BinaryOp(InstrKind::Shl, mul->getOperands()[0], shiftConst);
            replaceWithInstruction(mul, shlOp, bb, index);
            return true;
        }
        if (lhs && isPowerOfTwo(lhs->getValue())) {
            int shiftAmount = log2(lhs->getValue());
            auto shiftConst = new Constant(shiftAmount, std::to_string(shiftAmount));
            auto shlOp = new BinaryOp(InstrKind::Shl, mul->getOperands()[1], shiftConst);
            replaceWithInstruction(mul, shlOp, bb, index);
            return true;
        }

        return false;
    }

    // And Peephole 1: And with 0 -> 0
    // And Peephole 2: And with all-ones -> identity
    // And Peephole 3: And with same value -> identity

    static bool applyAndPeepholes(BinaryOp* andOp, BasicBlock& bb, size_t index) {
        if (andOp->getKind() != InstrKind::And) return false;

        auto lhs = dynamic_cast<Constant*>(andOp->getOperands()[0]);
        auto rhs = dynamic_cast<Constant*>(andOp->getOperands()[1]);

        if (lhs && lhs->getValue() == 0) {
            replaceWithConstant(andOp, 0, bb, index);
            return true;
        }
        if (rhs && rhs->getValue() == 0) {
            replaceWithConstant(andOp, 0, bb, index);
            return true;
        }

        if (lhs && lhs->getValue() == -1) {
            replaceWithOperand(andOp, andOp->getOperands()[1], bb, index);
            return true;
        }
        if (rhs && rhs->getValue() == -1) {
            replaceWithOperand(andOp, andOp->getOperands()[0], bb, index);
            return true;
        }

        if (andOp->getOperands()[0] == andOp->getOperands()[1]) {
            replaceWithOperand(andOp, andOp->getOperands()[0], bb, index);
            return true;
        }

        return false;
    }

    // Shr Peephole 1: Shift by 0 -> identity
    // Shr Peephole 2: Shift by more than 31 bits -> 0 (for 32-bit integers)
    // Shr Peephole 3: Shift of 0 -> 0
    static bool applyShrPeepholes(BinaryOp* shr, BasicBlock& bb, size_t index) {
        if (shr->getKind() != InstrKind::Shr) return false;

        auto shiftAmount = dynamic_cast<Constant*>(shr->getOperands()[1]);
        if (!shiftAmount) return false;

        int shiftVal = shiftAmount->getValue();

        if (shiftVal == 0) {
            replaceWithOperand(shr, shr->getOperands()[0], bb, index);
            return true;
        }

        if (shiftVal >= 32) {
            replaceWithConstant(shr, 0, bb, index);
            return true;
        }

        auto shiftedValue = dynamic_cast<Constant*>(shr->getOperands()[0]);
        if (shiftedValue && shiftedValue->getValue() == 0) {
            replaceWithConstant(shr, 0, bb, index);
            return true;
        }

        return false;
    }

    static void replaceWithConstant(BinaryOp* op, int constantValue, BasicBlock& bb, size_t index) {
        auto constant = std::make_unique<ConstantInstruction>(constantValue);
        replaceInstruction(bb, index, op, std::move(constant));
    }

    static void replaceWithOperand(BinaryOp* op, Value* operand, BasicBlock& bb, size_t index) {
        if (auto* constant = dynamic_cast<Constant*>(operand)) {
            auto replacement = std::make_unique<ConstantInstruction>(constant->getValue(), constant->getName());
            replaceInstruction(bb, index, op, std::move(replacement));
            return;
        }
        replaceUsesAfter(bb, index, op, operand);
        op->dropAllOperands();
        bb.getInstructions().erase(bb.getInstructions().begin() + static_cast<std::ptrdiff_t>(index));
    }

    static void replaceWithInstruction(BinaryOp* oldOp, Instruction* newInstr, BasicBlock& bb, size_t index) {
        replaceInstruction(bb, index, oldOp, std::unique_ptr<Instruction>(newInstr));
    }

    static void replaceInstruction(BasicBlock& bb,
                                   size_t index,
                                   Instruction* oldInstr,
                                   std::unique_ptr<Instruction> newInstr) {
        Instruction* replacement = newInstr.get();
        replaceUsesAfter(bb, index, oldInstr, replacement);
        oldInstr->dropAllOperands();
        bb.getInstructions()[index] = std::move(newInstr);
    }

    static void replaceUsesAfter(BasicBlock& bb, size_t index, Value* oldValue, Value* newValue) {
        auto& instructions = bb.getInstructions();
        for (size_t i = index + 1; i < instructions.size(); ++i) {
            instructions[i]->replaceOperand(oldValue, newValue);
        }
    }

    static bool isPowerOfTwo(int x) {
        return x > 0 && (x & (x - 1)) == 0;
    }

    static int log2(int x) {
        int result = 0;
        while (x > 1) {
            x >>= 1;
            result++;
        }
        return result;
    }
};

class PeepholePass {
public:
    static bool runOnFunction(Function& function) {
        bool changed = false;

        for (auto& bb : function.getBasicBlocks()) {
            if (PeepholeOptimizer::optimizeBlock(*bb)) {
                changed = true;
            }
        }

        return changed;
    }
};
