#pragma once

#include "function.h"
#include "instruction.h"
#include "context.h"
#include <sstream>

class Call : public Instruction {
    Function* callee;

public:
    Call(Function* target, const std::vector<Value*>& args)
        : Instruction(InstrKind::Call, args), callee(target) {}

    Function* getCallee() const { return callee; }

    std::string str(NameContext& ctx) const override {
        std::ostringstream out;
        out << ctx.getValueName(this) << " = call ";
        out << (callee ? callee->getName() : "<unresolved>") << "(";
        for (size_t i = 0; i < operands.size(); ++i) {
            if (i != 0) {
                out << ", ";
            }
            out << ctx.getValueName(operands[i]);
        }
        out << ")";
        return out.str();
    }

    void updateCFG() override {}

    void print(std::ostream& os) const override {
        os << "call " << (callee ? callee->getName() : "<unresolved>");
    }
};
