#pragma once

#include "bin_ops.h"
#include "call.h"
#include "cfg.h"
#include "constant_folding.h"
#include "control_flow.h"
#include "peephole_optimizer.h"
#include "program.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class StaticInlinerPass {
public:
    struct Config {
        size_t maxCalleeInstructions;
        size_t maxTotalInstructions;
        bool runLocalOptimizations;
        bool allowSmallOverLimit;

        Config()
            : maxCalleeInstructions(32),
              maxTotalInstructions(256),
              runLocalOptimizations(true),
              allowSmallOverLimit(true) {}
    };

    static bool runOnProgram(Program& program, const Config& config = Config()) {
        bool changed = false;
        for (auto& caller : program.getFunctions()) {
            changed |= runOnFunction(*caller, config);
        }
        return changed;
    }

    static bool runOnFunction(Function& caller, const Config& config = Config()) {
        bool changed = false;
        bool inlinedOne = false;
        do {
            inlinedOne = false;
            for (auto& block : caller.getBasicBlocks()) {
                auto& instructions = block->getInstructions();
                for (size_t i = 0; i < instructions.size(); ++i) {
                    auto* call = dynamic_cast<Call*>(instructions[i].get());
                    if (!call) {
                        continue;
                    }
                    if (!canInline(caller, *call, config)) {
                        continue;
                    }
                    inlineCall(caller, *block, i, *call, config);
                    changed = true;
                    inlinedOne = true;
                    break;
                }
                if (inlinedOne) {
                    break;
                }
            }
        } while (inlinedOne);
        return changed;
    }

    static bool canInline(const Function& caller, const Call& call, const Config& config = Config()) {
        Function* callee = call.getCallee();
        if (!callee || callee == &caller || callee->isNative() || callee->isInlineBlacklisted()) {
            return false;
        }
        if (callee->isExternal() && !isAotExternalInlineCandidate(*callee)) {
            return false;
        }
        if (callee->getParams().size() != call.getOperands().size()) {
            return false;
        }

        const size_t calleeInstructionCount = countInstructions(*callee);
        if (calleeInstructionCount > config.maxCalleeInstructions) {
            return false;
        }

        const size_t projected = countInstructions(caller) + calleeInstructionCount;
        if (projected > config.maxTotalInstructions && !isSmallException(calleeInstructionCount, config)) {
            return false;
        }

        return true;
    }

private:
    struct CloneState {
        std::unordered_map<const BasicBlock*, BasicBlock*> blocks;
        std::unordered_map<std::string, std::string> blockNames;
        std::unordered_map<const Value*, Value*> values;
        std::vector<std::pair<BasicBlock*, Value*>> returns;
    };

    static size_t& inlineCounter() {
        static size_t counter = 0;
        return counter;
    }

    static size_t countInstructions(const Function& function) {
        size_t count = 0;
        for (const auto& block : function.getBasicBlocks()) {
            count += block->getInstructions().size();
        }
        return count;
    }

    static bool isSmallException(size_t calleeInstructionCount, const Config& config) {
        return config.allowSmallOverLimit && calleeInstructionCount <= 4;
    }

    static bool isAotExternalInlineCandidate(const Function& callee) {
        return countInstructions(callee) <= 4;
    }

    static void inlineCall(Function& caller,
                           BasicBlock& callBlock,
                           size_t callIndex,
                           Call& call,
                           const Config& config) {
        Function& callee = *call.getCallee();
        Value* oldCallValue = &call;
        std::vector<Value*> arguments = call.getOperands();
        const std::string prefix = "__inline" + std::to_string(++inlineCounter()) + "_" + callee.getName() + "_";
        BasicBlock* continuation = splitCallBlock(caller, callBlock, callIndex, prefix + "cont");

        CloneState state;
        mapParametersAndConstants(caller, callee, arguments, state);
        cloneBlocks(caller, callee, prefix, state);
        cloneInstructions(caller, callee, continuation, state);
        wireCallBlock(callBlock, callee, state);
        Value* replacement = buildReturnValue(continuation, state);
        replaceAllUses(caller, oldCallValue, replacement);
        removeUnreachableBlocks(caller);
        CFGAnalysis::buildCFG(caller);

        if (config.runLocalOptimizations) {
            PeepholePass::runOnFunction(caller);
            ConstantFoldingPass::runOnFunction(caller);
        }
    }

    static BasicBlock* splitCallBlock(Function& caller,
                                      BasicBlock& callBlock,
                                      size_t callIndex,
                                      const std::string& continuationName) {
        BasicBlock* continuation = caller.createBasicBlock(continuationName);
        auto& callInstructions = callBlock.getInstructions();
        auto& continuationInstructions = continuation->getInstructions();

        for (size_t i = callIndex + 1; i < callInstructions.size(); ++i) {
            continuationInstructions.push_back(std::move(callInstructions[i]));
        }
        callInstructions.erase(callInstructions.begin() + callIndex, callInstructions.end());
        return continuation;
    }

    static void mapParametersAndConstants(Function& caller,
                                          const Function& callee,
                                          const std::vector<Value*>& arguments,
                                          CloneState& state) {
        for (size_t i = 0; i < callee.getParams().size(); ++i) {
            state.values[callee.getParams()[i].get()] = arguments[i];
        }
        for (const auto& constant : callee.getConstants()) {
            state.values[constant.get()] = caller.createConstant(constant->getValue(), constant->getName());
        }
    }

    static void cloneBlocks(Function& caller,
                            const Function& callee,
                            const std::string& prefix,
                            CloneState& state) {
        for (const auto& block : callee.getBasicBlocks()) {
            BasicBlock* cloned = caller.createBasicBlock(prefix + block->getName());
            state.blocks[block.get()] = cloned;
            state.blockNames[block->getName()] = cloned->getName();
        }
    }

    static void cloneInstructions(Function& caller,
                                  const Function& callee,
                                  BasicBlock* continuation,
                                  CloneState& state) {
        for (const auto& oldBlock : callee.getBasicBlocks()) {
            BasicBlock* newBlock = state.blocks[oldBlock.get()];
            for (const auto& oldInstr : oldBlock->getInstructions()) {
                if (auto* ret = dynamic_cast<Return*>(oldInstr.get())) {
                    if (ret->getReturnValue()) {
                        state.returns.emplace_back(newBlock, mapValue(caller, ret->getReturnValue(), state));
                    }
                    newBlock->createInstr<Jump>(continuation->getName());
                    continue;
                }
                Instruction* cloned = cloneInstruction(caller, *oldInstr, state);
                if (cloned) {
                    newBlock->getInstructions().push_back(std::unique_ptr<Instruction>(cloned));
                    state.values[oldInstr.get()] = cloned;
                }
            }
        }
    }

    static Instruction* cloneInstruction(Function& caller, const Instruction& oldInstr, CloneState& state) {
        if (auto* binary = dynamic_cast<const BinaryOp*>(&oldInstr)) {
            return new BinaryOp(binary->getKind(),
                                mapValue(caller, binary->getOperands()[0], state),
                                mapValue(caller, binary->getOperands()[1], state));
        }
        if (auto* cmp = dynamic_cast<const Cmp*>(&oldInstr)) {
            return new Cmp(cmp->getCmpOp(),
                           mapValue(caller, cmp->getOperands()[0], state),
                           mapValue(caller, cmp->getOperands()[1], state));
        }
        if (auto* jump = dynamic_cast<const Jump*>(&oldInstr)) {
            return new Jump(mappedBlockName(jump->getTargetName(), state));
        }
        if (auto* condJump = dynamic_cast<const CondJump*>(&oldInstr)) {
            return new CondJump(condJump->getCondition(),
                                mappedBlockName(condJump->getTrueTargetName(), state),
                                mappedBlockName(condJump->getFalseTargetName(), state));
        }
        if (auto* phi = dynamic_cast<const Phi*>(&oldInstr)) {
            auto* clonedPhi = new Phi();
            state.values[&oldInstr] = clonedPhi;
            for (const auto& incoming : phi->getIncoming()) {
                clonedPhi->addIncoming(state.blocks[incoming.first], mapValue(caller, incoming.second, state));
            }
            return clonedPhi;
        }
        return nullptr;
    }

    static Value* mapValue(Function& caller, Value* oldValue, CloneState& state) {
        auto it = state.values.find(oldValue);
        if (it != state.values.end()) {
            return it->second;
        }
        if (auto* constant = dynamic_cast<Constant*>(oldValue)) {
            Value* cloned = caller.createConstant(constant->getValue(), constant->getName());
            state.values[oldValue] = cloned;
            return cloned;
        }
        return oldValue;
    }

    static std::string mappedBlockName(const std::string& oldBlockName, const CloneState& state) {
        auto it = state.blockNames.find(oldBlockName);
        return it == state.blockNames.end() ? oldBlockName : it->second;
    }

    static void wireCallBlock(BasicBlock& callBlock, const Function& callee, const CloneState& state) {
        if (callee.getBasicBlocks().empty()) {
            return;
        }
        const BasicBlock* calleeEntry = callee.getBasicBlocks().front().get();
        callBlock.createInstr<Jump>(state.blocks.at(calleeEntry)->getName());
    }

    static Value* buildReturnValue(BasicBlock* continuation, CloneState& state) {
        if (state.returns.empty()) {
            return nullptr;
        }
        if (state.returns.size() == 1) {
            return state.returns.front().second;
        }

        auto phi = std::make_unique<Phi>();
        Phi* phiPtr = phi.get();
        for (auto& ret : state.returns) {
            phiPtr->addIncoming(ret.first, ret.second);
        }
        continuation->getInstructions().insert(continuation->getInstructions().begin(), std::move(phi));
        return phiPtr;
    }

    static void replaceAllUses(Function& function, Value* oldValue, Value* newValue) {
        if (!newValue) {
            return;
        }
        for (auto& block : function.getBasicBlocks()) {
            for (auto& instr : block->getInstructions()) {
                instr->replaceOperand(oldValue, newValue);
                if (auto* phi = dynamic_cast<Phi*>(instr.get())) {
                    phi->replaceIncomingValue(oldValue, newValue);
                }
            }
        }
    }

    static void removeUnreachableBlocks(Function& function) {
        if (function.getBasicBlocks().empty()) {
            return;
        }

        CFGAnalysis::buildCFG(function);
        std::unordered_set<BasicBlock*> reachable;
        std::vector<BasicBlock*> stack{function.getBasicBlocks().front().get()};
        while (!stack.empty()) {
            BasicBlock* current = stack.back();
            stack.pop_back();
            if (!reachable.insert(current).second) {
                continue;
            }
            for (BasicBlock* successor : current->getSuccessors()) {
                stack.push_back(successor);
            }
        }

        auto& blocks = function.getBasicBlocks();
        blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                                    [&](const auto& block) {
                                        return reachable.count(block.get()) == 0;
                                    }),
                     blocks.end());
    }
};
