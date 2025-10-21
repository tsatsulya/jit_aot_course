#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include "context.h"
#include "cfg.h"

#include "function.h"
class Program {
    std::vector<std::unique_ptr<Function>> functions;

public:
    Function& createFunction(const std::string& name) {
        functions.push_back(std::make_unique<Function>(name));
        return *functions.back();
    }

    void dumpBasicBlock(const BasicBlock& bb, NameContext& ctx) const {
        std::cout << bb.getName() << ":\n";
        int instr_count = 0;
        for (const auto& instr : bb.getInstructions()) {
            try {
                std::cout << "  " << instr->str(ctx) << "\n";
            } catch (...) {
                std::cout << "  [CRASHED on instruction " << instr_count << "]\n";
            }
            instr_count++;
        }
    }

    void dumpFunctionCFG(const Function& func) const {
        NameContext ctx;
        std::cout << "Function " << func.getName() << " CFG:\n";

        for (const auto& bb : func.getBasicBlocks()) {
            std::cout << bb->getName() << ":\n";
            std::cout << "  Predecessors: ";
            for (auto pred : bb->getPredecessors()) {
                std::cout << pred->getName() << " ";
            }
            std::cout << "\n  Successors: ";
            for (auto succ : bb->getSuccessors()) {
                std::cout << succ->getName() << " ";
            }
            std::cout << "\n";

            dumpBasicBlock(*bb, ctx);
            std::cout << "\n";
        }
    }

    void dumpFunction(const Function& func) const {
        NameContext ctx;
        std::cout << "\n\nFunction " << func.getName() << "(";
        for (size_t i = 0; i < func.getParams().size(); ++i) {
            std::cout << func.getParams()[i]->str(ctx);
            if (i + 1 < func.getParams().size()) std::cout << ", ";
        }
        std::cout << ")\n";

        for (const auto& bb : func.getBasicBlocks()) {
            dumpBasicBlock(*bb, ctx);
        }
    }

    void dump() const {
        for (const auto& func : functions) {
            dumpFunction(*func);
        }
    }

    void dumpWithCFG() const {
        for (const auto& func : functions) {
            dumpFunctionCFG(*func);
        }
    }

    std::vector<std::unique_ptr<Function>> &getFunctions() {
        return functions;
    }
};