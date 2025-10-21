#pragma once

#include "basic_block.h"
#include "control_flow.h"
#include "function.h"
#include <unordered_set>
#include <queue>

class CFGAnalysis {
public:
    static void buildCFG(Function& function) {
        for (auto& bb : function.getBasicBlocks()) {
            for (auto& instr : bb->getInstructions()) {
                if (auto jump = dynamic_cast<Jump*>(instr.get())) {
                    auto target = findBlockByName(function, jump->getTargetName());
                    if (target) {
                        jump->setTargetBlock(target);
                        jump->setParentBlock(bb.get());
                    }
                }
                else if (auto condJump = dynamic_cast<CondJump*>(instr.get())) {
                    auto trueTarget = findBlockByName(function, condJump->getTrueTargetName());
                    auto falseTarget = findBlockByName(function, condJump->getFalseTargetName());

                    if (trueTarget && falseTarget) {
                        condJump->setTrueTargetBlock(trueTarget);
                        condJump->setFalseTargetBlock(falseTarget);
                        condJump->setParentBlock(bb.get());
                    }
                }
            }
        }

        for (auto& bb : function.getBasicBlocks()) {
            bb->clearPredecessors();
            bb->clearSuccessors();
        }

        for (auto& bb : function.getBasicBlocks()) {
            auto& instructions = bb->getInstructions();
            if (!instructions.empty()) {
                auto& lastInstr = instructions.back();
                lastInstr->updateCFG();
            }
        }
    }


    static std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>
    computeDominators(Function& function) {
        std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>> dominators;
        auto& basicBlocks = function.getBasicBlocks();

        if (basicBlocks.empty()) return dominators;

        BasicBlock* entry = basicBlocks[0].get();

        for (auto& bb : basicBlocks) {
            if (bb.get() == entry) {
                dominators[entry] = {entry};
            } else {
                std::unordered_set<BasicBlock*> allBlocks;
                for (auto& b : basicBlocks) {
                    allBlocks.insert(b.get());
                }
                dominators[bb.get()] = allBlocks;
            }
        }

        bool changed;
        do {
            changed = false;
            for (auto& bb : basicBlocks) {
                if (bb.get() == entry) continue;

                auto& preds = bb->getPredecessors();
                if (preds.empty()) continue;

                // Start with the first predecessor's dominators
                auto firstPred = *preds.begin();
                std::unordered_set<BasicBlock*> newDoms = dominators[firstPred];

                // Intersect with all other predecessors
                for (auto pred : preds) {
                    if (pred == firstPred) continue;

                    std::unordered_set<BasicBlock*> temp;
                    for (auto dom : newDoms) {
                        if (dominators[pred].count(dom)) {
                            temp.insert(dom);
                        }
                    }
                    newDoms = std::move(temp);
                }

                // Add the block itself
                newDoms.insert(bb.get());

                if (newDoms != dominators[bb.get()]) {
                    dominators[bb.get()] = newDoms;
                    changed = true;
                }
            }
        } while (changed);

        return dominators;
    }

    static void dfsTraversal(BasicBlock* start,
                            std::unordered_set<BasicBlock*>& visited,
                            std::vector<BasicBlock*>& order) {
        if (!start || visited.count(start)) return;

        visited.insert(start);
        order.push_back(start);

        for (auto succ : start->getSuccessors()) {
            dfsTraversal(succ, visited, order);
        }
    }
private:
    static BasicBlock* findBlockByName(Function& function, const std::string& name) {
        for (auto& bb : function.getBasicBlocks()) {
            if (bb->getName() == name) {
                return bb.get();
            }
        }
        return nullptr;
    }
};