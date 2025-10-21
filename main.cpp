#include "program.h"
#include "bin_ops.h"
#include "control_flow.h"
#include <iostream>

void createCompleteFactorial(Program& program) {
    Function& fact = program.createFunction("just_factorial");
    Parameter* n = fact.createParam("n");

    BasicBlock* entry = fact.createBasicBlock("entry");
    BasicBlock* check = fact.createBasicBlock("check");
    BasicBlock* base_case = fact.createBasicBlock("base_case");
    BasicBlock* recursive = fact.createBasicBlock("recursive");
    BasicBlock* compute = fact.createBasicBlock("compute");
    BasicBlock* exit = fact.createBasicBlock("exit");

    Constant* one = fact.createConstant(1, "one");
    [[maybe_unused]] Constant* zero = fact.createConstant(0, "zero");

    entry->createInstr<Jump>("check");

    auto& cmp = check->createInstr<Cmp>(CmpOp::Le, n, one);
    NameContext ctx;
    std::string condStr = cmp.str(ctx);
    check->createInstr<CondJump>(condStr, "base_case", "recursive");

    base_case->createInstr<Return>(one);

    auto& n_minus_one = recursive->createInstr<BinaryOp>(InstrKind::Sub, n, one);
    recursive->createInstr<Jump>("compute");

    auto& computed_result = compute->createInstr<BinaryOp>(InstrKind::Mul, n, &n_minus_one);
    compute->createInstr<Jump>("exit");

    exit->createInstr<Return>(&computed_result);
}

void createIterativeFactorial(Program& program) {
    Function& fact = program.createFunction("iterative_factorial");
    Parameter* n = fact.createParam("n");

    BasicBlock* entry = fact.createBasicBlock("entry");
    BasicBlock* loop_check = fact.createBasicBlock("loop_check");
    BasicBlock* loop_body = fact.createBasicBlock("loop_body");
    BasicBlock* exit = fact.createBasicBlock("exit");

    Constant* one = fact.createConstant(1, "one");
    Constant* zero = fact.createConstant(0, "zero");

    auto& result = entry->createInstr<BinaryOp>(InstrKind::Add, one, zero);
    auto& i = entry->createInstr<BinaryOp>(InstrKind::Add, n, zero);
    entry->createInstr<Jump>("loop_check");

    auto& cmp = loop_check->createInstr<Cmp>(CmpOp::Gt, &i, zero);
    NameContext ctx;
    std::string condStr = cmp.str(ctx);
    loop_check->createInstr<CondJump>(condStr, "loop_body", "exit");

    auto& new_result = loop_body->createInstr<BinaryOp>(InstrKind::Mul, &result, &i);
    [[maybe_unused]] auto& new_i = loop_body->createInstr<BinaryOp>(InstrKind::Sub, &i, one);
    loop_body->createInstr<Jump>("loop_check");

    exit->createInstr<Return>(&new_result);
}

void createSimpleFactorial(Program& program) {
    Function& fact = program.createFunction("simple_factorial");
    fact.createParam("n");

    BasicBlock* entry = fact.createBasicBlock("entry");
    [[maybe_unused]] BasicBlock* exit = fact.createBasicBlock("exit");

    Constant* one = fact.createConstant(1, "one");
    entry->createInstr<Return>(one);
}

void createConditionalFactorial(Program& program) {

    Function& fact = program.createFunction("conditional_factorial");
    Parameter* n = fact.createParam("n");

    BasicBlock* entry = fact.createBasicBlock("entry");
    BasicBlock* check = fact.createBasicBlock("check");
    BasicBlock* base = fact.createBasicBlock("base");
    BasicBlock* recurse = fact.createBasicBlock("recurse");

    Constant* one = fact.createConstant(1, "one");
    [[maybe_unused]] Constant* zero = fact.createConstant(0, "zero");

    entry->createInstr<Jump>("check");

    auto& cmp = check->createInstr<Cmp>(CmpOp::Le, n, one);
    NameContext ctx;
    std::string condStr = cmp.str(ctx);
    check->createInstr<CondJump>(condStr, "base", "recurse");

    base->createInstr<Return>(one);

    auto& n_minus_one = recurse->createInstr<BinaryOp>(InstrKind::Sub, n, one);
    auto& result = recurse->createInstr<BinaryOp>(InstrKind::Mul, n, &n_minus_one);
    recurse->createInstr<Return>(&result);
}

int main() {
    Program program;

    createSimpleFactorial(program);
    createConditionalFactorial(program);
    createCompleteFactorial(program);
    createIterativeFactorial(program);

    for (auto& func : program.getFunctions()) {
        CFGAnalysis::buildCFG(*func);
    }

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "BASIC FUNCTION DUMP:\n";
    std::cout << std::string(60, '=') << "\n";
    program.dump();

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "CFG ANALYSIS:\n";
    std::cout << std::string(60, '=') << "\n";
    program.dumpWithCFG();
    return 0;
}