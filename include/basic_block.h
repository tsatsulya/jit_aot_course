#pragma once


#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "instruction.h"

class NameContext;

class BasicBlock {
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;

public:
    BasicBlock(const std::string& nm) : name(nm) {}

    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const { return instructions; }

    template<typename Instr, typename... Args>
    Instr& createInstr(Args&&... args) {
        instructions.push_back(std::make_unique<Instr>(std::forward<Args>(args)...));
        return static_cast<Instr&>(*instructions.back());
    }
};