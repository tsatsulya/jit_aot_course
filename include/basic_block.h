#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_set>

#include "instruction.h"

class NameContext;

class BasicBlock {
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unordered_set<BasicBlock*> predecessors;
    std::unordered_set<BasicBlock*> successors;

public:
    BasicBlock(const std::string& nm) : name(nm) {}

    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const { return instructions; }
    std::vector<std::unique_ptr<Instruction>>& getInstructions() { return instructions; }
    const std::unordered_set<BasicBlock*>& getPredecessors() const { return predecessors; }
    const std::unordered_set<BasicBlock*>& getSuccessors() const { return successors; }

    template<typename Instr, typename... Args>
    Instr& createInstr(Args&&... args) {
        instructions.push_back(std::make_unique<Instr>(std::forward<Args>(args)...));
        return static_cast<Instr&>(*instructions.back());
    }

    void addPredecessor(BasicBlock* pred) { predecessors.insert(pred); }
    void addSuccessor(BasicBlock* succ) { successors.insert(succ); }
    void removePredecessor(BasicBlock* pred) { predecessors.erase(pred); }
    void removeSuccessor(BasicBlock* succ) { successors.erase(succ); }

    void clearPredecessors() { predecessors.clear(); }
    void clearSuccessors() { successors.clear(); }
};