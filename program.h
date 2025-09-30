#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "function.h"

class Program {
    std::vector<std::unique_ptr<Function>> functions;
    std::unordered_map<const Value*, std::string> valueNames;
    int nextValueId = 0;

    void resetValueNames() {
        valueNames.clear();
        nextValueId = 0;
    }

    std::string getValueName(const Value* val) {
        auto it = valueNames.find(val);
        if (it != valueNames.end()) return it->second;
        std::string name = "%v" + std::to_string(nextValueId++);
        valueNames[val] = name;
        return name;
    }

public:
    Function& createFunction(const std::string& name) {
        functions.push_back(std::make_unique<Function>(name));
        return *functions.back();
    }


    void dumpBasicBlock(const BasicBlock& bb, NameContext& ctx) const{
        std::cout << bb.getName() << ":\n";
        for (const auto& instr : bb.getInstructions()) {
            std::cout << "  " << instr->str(ctx) << "\n";
        }
    }

    void dumpFunction(const Function& func) const {
        NameContext ctx;
        std::cout << "Function " << func.getName() << "(";
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
};

