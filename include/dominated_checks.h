#pragma once

#include "basic_block.h"
#include "cfg.h"
#include "checks.h"
#include "function.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class DominatedCheckEliminationPass {
public:
    static bool runOnFunction(Function& function) {
        CFGAnalysis::buildCFG(function);

        auto dominators = CFGAnalysis::computeDominators(function);
        PositionMap positions = buildPositionMap(function);
        std::vector<BasicBlock*> order = computeRPO(function);
        appendUnvisitedBlocks(function, order);

        bool changed = false;
        for (BasicBlock* block : order) {
            auto& instructions = block->getInstructions();
            for (size_t i = 0; i < instructions.size(); ++i) {
                Instruction* check = instructions[i].get();
                if (!isCheck(check)) {
                    continue;
                }
                if (hasDominatingEquivalentCheck(check, positions, dominators)) {
                    check->dropAllOperands();
                    instructions.erase(instructions.begin() + static_cast<std::ptrdiff_t>(i));
                    positions = buildPositionMap(function);
                    changed = true;
                    --i;
                }
            }
        }

        return changed;
    }

private:
    struct InstrPosition {
        BasicBlock* block;
        size_t index;
    };

    using PositionMap = std::unordered_map<const Instruction*, InstrPosition>;

    static bool isCheck(const Instruction* instr) {
        return instr && (instr->getKind() == InstrKind::NullCheck ||
                         instr->getKind() == InstrKind::BoundsCheck);
    }

    static Value* checkedObject(Instruction* check) {
        if (!isCheck(check) || check->getOperands().empty()) {
            return nullptr;
        }
        return check->getOperands()[0];
    }

    static bool sameCheck(const Instruction* lhs, const Instruction* rhs) {
        if (!isCheck(lhs) || !isCheck(rhs) || lhs->getKind() != rhs->getKind()) {
            return false;
        }
        const auto& lhsOperands = lhs->getOperands();
        const auto& rhsOperands = rhs->getOperands();
        return lhsOperands.size() == rhsOperands.size() &&
               std::equal(lhsOperands.begin(), lhsOperands.end(), rhsOperands.begin());
    }

    static PositionMap buildPositionMap(Function& function) {
        PositionMap positions;
        for (auto& block : function.getBasicBlocks()) {
            auto& instructions = block->getInstructions();
            for (size_t i = 0; i < instructions.size(); ++i) {
                positions[instructions[i].get()] = {block.get(), i};
            }
        }
        return positions;
    }

    static bool dominates(const Instruction* candidate,
                          const Instruction* target,
                          const PositionMap& positions,
                          const std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& dominators) {
        auto candidateIt = positions.find(candidate);
        auto targetIt = positions.find(target);
        if (candidateIt == positions.end() || targetIt == positions.end()) {
            return false;
        }

        BasicBlock* candidateBlock = candidateIt->second.block;
        BasicBlock* targetBlock = targetIt->second.block;
        if (candidateBlock == targetBlock) {
            return candidateIt->second.index < targetIt->second.index;
        }

        auto domIt = dominators.find(targetBlock);
        return domIt != dominators.end() && domIt->second.count(candidateBlock) != 0;
    }

    static bool hasDominatingEquivalentCheck(
        Instruction* check,
        const PositionMap& positions,
        const std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& dominators) {
        Value* object = checkedObject(check);
        if (!object) {
            return false;
        }

        std::vector<Instruction*> users = object->getUsers();
        for (Instruction* user : users) {
            if (user == check || !sameCheck(user, check)) {
                continue;
            }
            if (dominates(user, check, positions, dominators)) {
                return true;
            }
        }
        return false;
    }

    static std::vector<BasicBlock*> computeRPO(Function& function) {
        std::vector<BasicBlock*> postOrder;
        std::unordered_set<BasicBlock*> visited;
        if (!function.getBasicBlocks().empty()) {
            dfsPostOrder(function.getBasicBlocks().front().get(), visited, postOrder);
        }
        std::reverse(postOrder.begin(), postOrder.end());
        return postOrder;
    }

    static void dfsPostOrder(BasicBlock* block,
                             std::unordered_set<BasicBlock*>& visited,
                             std::vector<BasicBlock*>& postOrder) {
        if (!block || !visited.insert(block).second) {
            return;
        }
        for (BasicBlock* succ : block->getSuccessors()) {
            dfsPostOrder(succ, visited, postOrder);
        }
        postOrder.push_back(block);
    }

    static void appendUnvisitedBlocks(Function& function, std::vector<BasicBlock*>& order) {
        std::unordered_set<BasicBlock*> seen(order.begin(), order.end());
        for (auto& block : function.getBasicBlocks()) {
            if (seen.insert(block.get()).second) {
                order.push_back(block.get());
            }
        }
    }
};
