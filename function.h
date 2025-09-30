#pragma once

#include <memory>
#include <iostream>


#include "instruction.h"
#include "basic_block.h"

class Function {
    std::string name;
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::vector<std::unique_ptr<BasicBlock>> basicBlocks;

public:
    Function(const std::string& nm) : name(nm) {}

    Parameter& createParam(const std::string& nm) {
        parameters.push_back(std::make_unique<Parameter>(nm));
        return *parameters.back();
    }

    BasicBlock& createBasicBlock(const std::string& nm) {
        basicBlocks.push_back(std::make_unique<BasicBlock>(nm));
        return *basicBlocks.back();
    }

    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<Parameter>>& getParams() const { return parameters; }
    const std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() const { return basicBlocks; }
};