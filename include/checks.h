#pragma once

#include "context.h"
#include "instruction.h"

class NullCheck : public Instruction {
public:
    explicit NullCheck(Value* object) : Instruction(InstrKind::NullCheck, {object}) {}

    Value* getObject() const { return operands[0]; }

    std::string str(NameContext& ctx) const override {
        return "nullcheck " + ctx.getValueName(getObject());
    }

    void updateCFG() override {}

    void print(std::ostream& os) const override {
        os << "nullcheck " << getObject();
    }
};

class BoundsCheck : public Instruction {
public:
    BoundsCheck(Value* object, Value* index) : Instruction(InstrKind::BoundsCheck, {object, index}) {}

    Value* getObject() const { return operands[0]; }
    Value* getIndex() const { return operands[1]; }

    std::string str(NameContext& ctx) const override {
        return "boundscheck " + ctx.getValueName(getObject()) + ", " + ctx.getValueName(getIndex());
    }

    void updateCFG() override {}

    void print(std::ostream& os) const override {
        os << "boundscheck " << getObject() << ", " << getIndex();
    }
};
