#pragma once

#include <memory>
#include <iostream>
#include <algorithm>


#include "instruction.h"
#include "basic_block.h"

class Function {
    std::string name;
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::vector<std::unique_ptr<BasicBlock>> basicBlocks;
    std::vector<std::unique_ptr<Constant>> constants;
    bool native = false;
    bool external = false;
    bool inlineBlacklisted = false;

public:
    Function(const std::string& nm) : name(nm) {}

    Parameter* createParam(const std::string& nm) {
        parameters.push_back(std::make_unique<Parameter>(nm));
        return parameters.back().get();
    }

    BasicBlock* createBasicBlock(const std::string& nm) {
        basicBlocks.push_back(std::make_unique<BasicBlock>(nm));
        return basicBlocks.back().get();
    }

    Constant* createConstant(int value, const std::string& nm) {
        constants.push_back(std::make_unique<Constant>(value, nm));
        return constants.back().get();
    }

    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<Parameter>>& getParams() const { return parameters; }
    const std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() const { return basicBlocks; }
    std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() { return basicBlocks; }
    const std::vector<std::unique_ptr<Constant>>& getConstants() const { return constants; }

    void setNative(bool value) { native = value; }
    void setExternal(bool value) { external = value; }
    void setInlineBlacklisted(bool value) { inlineBlacklisted = value; }
    bool isNative() const { return native; }
    bool isExternal() const { return external; }
    bool isInlineBlacklisted() const { return inlineBlacklisted; }

    void removeBasicBlock(BasicBlock* block) {
        basicBlocks.erase(std::remove_if(basicBlocks.begin(), basicBlocks.end(),
                                         [block](const auto& bb) { return bb.get() == block; }),
                          basicBlocks.end());
    }
};
