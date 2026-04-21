#include <gtest/gtest.h>
#include "program.h"
#include "constant_folding.h"
#include "peephole_optimizer.h"
#include "bin_ops.h"
#include "call.h"
#include "checks.h"
#include "dominated_checks.h"
#include "static_inliner.h"

class OptimizationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        program = std::make_unique<Program>();
    }

    void TearDown() override {
        program.reset();
    }

    std::unique_ptr<Program> program;

    int countInstructions(const Function& func, InstrKind kind) {
        int count = 0;
        for (const auto& bb : func.getBasicBlocks()) {
            for (const auto& instr : bb->getInstructions()) {
                if (auto binaryOp = dynamic_cast<BinaryOp*>(instr.get())) {
                    if (binaryOp->getKind() == kind) {
                        count++;
                    }
                }
            }
        }
        return count;
    }

    bool containsConstant(const BasicBlock& bb, int value) {
        for (const auto& instr : bb.getInstructions()) {
            if (auto constant = dynamic_cast<ConstantInstruction*>(instr.get())) {
                if (constant->getConstant()->getValue() == value) {
                    return true;
                }
            }
        }
        return false;
    }

    int countCalls(const Function& func) {
        int count = 0;
        for (const auto& bb : func.getBasicBlocks()) {
            for (const auto& instr : bb->getInstructions()) {
                if (dynamic_cast<Call*>(instr.get())) {
                    count++;
                }
            }
        }
        return count;
    }

    BasicBlock* findBlock(Function& func, const std::string& namePart) {
        for (auto& bb : func.getBasicBlocks()) {
            if (bb->getName().find(namePart) != std::string::npos) {
                return bb.get();
            }
        }
        return nullptr;
    }

    int countPhiInstructions(const Function& func) {
        int count = 0;
        for (const auto& bb : func.getBasicBlocks()) {
            for (const auto& instr : bb->getInstructions()) {
                if (dynamic_cast<Phi*>(instr.get())) {
                    count++;
                }
            }
        }
        return count;
    }

    int countChecks(const Function& func, InstrKind kind) {
        int count = 0;
        for (const auto& bb : func.getBasicBlocks()) {
            for (const auto& instr : bb->getInstructions()) {
                if (instr->getKind() == kind) {
                    count++;
                }
            }
        }
        return count;
    }
};

TEST_F(OptimizationsTest, ConstantFoldingMulBasic) {
    Function& func = program->createFunction("test_mul");
    BasicBlock* bb = func.createBasicBlock("entry");

    Constant* c5 = func.createConstant(5, "5");
    Constant* c3 = func.createConstant(3, "3");
    bb->createInstr<BinaryOp>(InstrKind::Mul, c5, c3);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_TRUE(containsConstant(*bb, 15));
}

TEST_F(OptimizationsTest, ConstantFoldingAndBasic) {
    Function& func = program->createFunction("test_and");
    BasicBlock* bb = func.createBasicBlock("entry");

    Constant* c5 = func.createConstant(5, "5");
    Constant* c3 = func.createConstant(3, "3");
    bb->createInstr<BinaryOp>(InstrKind::And, c5, c3);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
    EXPECT_TRUE(containsConstant(*bb, 1));
}

TEST_F(OptimizationsTest, ConstantFoldingShrBasic) {
    Function& func = program->createFunction("test_shr");
    BasicBlock* bb = func.createBasicBlock("entry");

    Constant* c8 = func.createConstant(8, "8");
    Constant* c2 = func.createConstant(2, "2");
    bb->createInstr<BinaryOp>(InstrKind::Shr, c8, c2);

    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 1);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 0);
    EXPECT_TRUE(containsConstant(*bb, 2));
}

TEST_F(OptimizationsTest, ConstantFoldingShlBasic) {
    Function& func = program->createFunction("test_shl");
    BasicBlock* bb = func.createBasicBlock("entry");

    Constant* c2 = func.createConstant(2, "2");
    Constant* c3 = func.createConstant(3, "3");
    bb->createInstr<BinaryOp>(InstrKind::Shl, c2, c3);

    EXPECT_EQ(countInstructions(func, InstrKind::Shl), 1);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Shl), 0);
    EXPECT_TRUE(containsConstant(*bb, 16));
}

TEST_F(OptimizationsTest, ConstantFoldingNoFoldWhenNotConstants) {
    Function& func = program->createFunction("test_no_fold");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* c3 = func.createConstant(3, "3");

    bb->createInstr<BinaryOp>(InstrKind::Mul, param, c3);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);
}

TEST_F(OptimizationsTest, ConstantFoldingMultipleInstructions) {
    Function& func = program->createFunction("test_multiple");
    BasicBlock* bb = func.createBasicBlock("entry");

    Constant* c4 = func.createConstant(4, "4");
    Constant* c5 = func.createConstant(5, "5");
    Constant* c2 = func.createConstant(2, "2");

    bb->createInstr<BinaryOp>(InstrKind::Mul, c4, c5);  // 20
    bb->createInstr<BinaryOp>(InstrKind::And, c5, c2);  // 0 (5 & 2 = 0)
    bb->createInstr<BinaryOp>(InstrKind::Shr, c4, c2);  // 1 (4 >> 2 = 1)

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 1);

    ConstantFoldingPass::runOnFunction(func);

    // All should be folded
    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 0);

    EXPECT_TRUE(containsConstant(*bb, 20));
    EXPECT_TRUE(containsConstant(*bb, 0));
    EXPECT_TRUE(containsConstant(*bb, 1));
}

// Peephole Optimization Tests
TEST_F(OptimizationsTest, PeepholeMulByZero) {
    Function& func = program->createFunction("test_mul_zero");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* zero = func.createConstant(0, "0");

    bb->createInstr<BinaryOp>(InstrKind::Mul, param, zero);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);

    PeepholePass::runOnFunction(func);

    // Mul by zero should be replaced with constant 0
    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_TRUE(containsConstant(*bb, 0));
}

TEST_F(OptimizationsTest, PeepholeMulByOne) {
    Function& func = program->createFunction("test_mul_one");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* one = func.createConstant(1, "1");

    bb->createInstr<BinaryOp>(InstrKind::Mul, param, one);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
}

TEST_F(OptimizationsTest, PeepholeAndWithZero) {
    Function& func = program->createFunction("test_and_zero");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* zero = func.createConstant(0, "0");

    bb->createInstr<BinaryOp>(InstrKind::And, param, zero);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
    EXPECT_TRUE(containsConstant(*bb, 0));
}

TEST_F(OptimizationsTest, PeepholeAndWithMinusOne) {
    Function& func = program->createFunction("test_and_minus_one");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* minusOne = func.createConstant(-1, "-1");

    bb->createInstr<BinaryOp>(InstrKind::And, param, minusOne);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
}

TEST_F(OptimizationsTest, PeepholeShrByZero) {
    Function& func = program->createFunction("test_shr_zero");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* zero = func.createConstant(0, "0");

    bb->createInstr<BinaryOp>(InstrKind::Shr, param, zero);

    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 0);
}

TEST_F(OptimizationsTest, PeepholeShrByLargeAmount) {
    Function& func = program->createFunction("test_shr_large");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* large = func.createConstant(32, "32");

    bb->createInstr<BinaryOp>(InstrKind::Shr, param, large);

    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 0);
    EXPECT_TRUE(containsConstant(*bb, 0));
}

TEST_F(OptimizationsTest, PeepholeMulPowerOfTwoToShl) {
    Function& func = program->createFunction("test_mul_power_two");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* eight = func.createConstant(8, "8");  // 2^3

    bb->createInstr<BinaryOp>(InstrKind::Mul, param, eight);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::Shl), 0);

    PeepholePass::runOnFunction(func);

    // Mul by 8 should be converted to Shl by 3
    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::Shl), 1);
}

TEST_F(OptimizationsTest, PeepholeAndSameValue) {
    Function& func = program->createFunction("test_and_same");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");

    bb->createInstr<BinaryOp>(InstrKind::And, param, param);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
}

TEST_F(OptimizationsTest, CombinedOptimizations) {
    Function& func = program->createFunction("test_combined");
    BasicBlock* bb = func.createBasicBlock("entry");

    Constant* c4 = func.createConstant(4, "4");
    Constant* c0 = func.createConstant(0, "0");
    Constant* minusOne = func.createConstant(-1, "-1");

    bb->createInstr<BinaryOp>(InstrKind::Mul, c4, c0);
    bb->createInstr<BinaryOp>(InstrKind::And, c4, minusOne);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);

    PeepholePass::runOnFunction(func);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);

    EXPECT_TRUE(containsConstant(*bb, 0));
    EXPECT_TRUE(containsConstant(*bb, 4));
}

TEST_F(OptimizationsTest, MultipleBasicBlocks) {
    Function& func = program->createFunction("test_multiple_blocks");
    BasicBlock* bb1 = func.createBasicBlock("block1");
    BasicBlock* bb2 = func.createBasicBlock("block2");

    Constant* c5 = func.createConstant(5, "5");
    Constant* c3 = func.createConstant(3, "3");
    Constant* c2 = func.createConstant(2, "2");

    bb1->createInstr<BinaryOp>(InstrKind::Mul, c5, c3);  // 15
    bb2->createInstr<BinaryOp>(InstrKind::And, c5, c2);  // 0

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);

    EXPECT_TRUE(containsConstant(*bb1, 15));
    EXPECT_TRUE(containsConstant(*bb2, 0));
}

TEST_F(OptimizationsTest, ConstantFoldingEdgeCases) {
    Function& func = program->createFunction("test_edge_cases");
    BasicBlock* bb = func.createBasicBlock("entry");

    Constant* maxInt = func.createConstant(2147483647, "max_int");
    Constant* one = func.createConstant(1, "1");

    bb->createInstr<BinaryOp>(InstrKind::And, maxInt, one);  // 1

    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);

    ConstantFoldingPass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
    EXPECT_TRUE(containsConstant(*bb, 1));
}

TEST_F(OptimizationsTest, PeepholeMultipleOptimizationsInSequence) {
    Function& func = program->createFunction("test_sequence");
    BasicBlock* bb = func.createBasicBlock("entry");

    Parameter* param = func.createParam("x");
    Constant* zero = func.createConstant(0, "0");
    Constant* minusOne = func.createConstant(-1, "-1");

    bb->createInstr<BinaryOp>(InstrKind::Mul, param, zero);
    bb->createInstr<BinaryOp>(InstrKind::And, param, minusOne);
    bb->createInstr<BinaryOp>(InstrKind::Shr, param, zero);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 0);
}

TEST_F(OptimizationsTest, DominatedNullCheckInSameBlockIsRemoved) {
    Function& func = program->createFunction("dominated_null_same_block");
    Parameter* object = func.createParam("obj");
    BasicBlock* entry = func.createBasicBlock("entry");

    entry->createInstr<NullCheck>(object);
    entry->createInstr<NullCheck>(object);

    EXPECT_EQ(countChecks(func, InstrKind::NullCheck), 2);

    EXPECT_TRUE(DominatedCheckEliminationPass::runOnFunction(func));

    EXPECT_EQ(countChecks(func, InstrKind::NullCheck), 1);
    EXPECT_EQ(object->getUsers().size(), 1U);
}

TEST_F(OptimizationsTest, DominatedNullCheckAcrossBlocksIsRemoved) {
    Function& func = program->createFunction("dominated_null_across_blocks");
    Parameter* object = func.createParam("obj");
    BasicBlock* entry = func.createBasicBlock("entry");
    BasicBlock* body = func.createBasicBlock("body");

    entry->createInstr<NullCheck>(object);
    entry->createInstr<Jump>("body");
    body->createInstr<NullCheck>(object);

    CFGAnalysis::buildCFG(func);
    EXPECT_TRUE(DominatedCheckEliminationPass::runOnFunction(func));

    EXPECT_EQ(countChecks(func, InstrKind::NullCheck), 1);
}

TEST_F(OptimizationsTest, NonDominatingNullCheckIsKept) {
    Function& func = program->createFunction("non_dominating_null");
    Parameter* object = func.createParam("obj");
    BasicBlock* entry = func.createBasicBlock("entry");
    BasicBlock* left = func.createBasicBlock("left");
    BasicBlock* right = func.createBasicBlock("right");
    BasicBlock* merge = func.createBasicBlock("merge");

    entry->createInstr<CondJump>("cond", "left", "right");
    left->createInstr<NullCheck>(object);
    left->createInstr<Jump>("merge");
    right->createInstr<Jump>("merge");
    merge->createInstr<NullCheck>(object);

    CFGAnalysis::buildCFG(func);
    EXPECT_FALSE(DominatedCheckEliminationPass::runOnFunction(func));

    EXPECT_EQ(countChecks(func, InstrKind::NullCheck), 2);
}

TEST_F(OptimizationsTest, DominatedBoundsCheckRequiresSameObjectAndIndex) {
    Function& func = program->createFunction("dominated_bounds");
    Parameter* array = func.createParam("array");
    Parameter* index = func.createParam("i");
    Parameter* otherIndex = func.createParam("j");
    BasicBlock* entry = func.createBasicBlock("entry");

    entry->createInstr<BoundsCheck>(array, index);
    entry->createInstr<BoundsCheck>(array, index);
    entry->createInstr<BoundsCheck>(array, otherIndex);

    EXPECT_TRUE(DominatedCheckEliminationPass::runOnFunction(func));

    EXPECT_EQ(countChecks(func, InstrKind::BoundsCheck), 2);
    EXPECT_EQ(array->getUsers().size(), 2U);
}

TEST_F(OptimizationsTest, StaticInliningSingleReturnReplacesCallUses) {
    Function& callee = program->createFunction("add_one");
    Parameter* x = callee.createParam("x");
    Constant* one = callee.createConstant(1, "1");
    BasicBlock* calleeEntry = callee.createBasicBlock("entry");
    auto& sum = calleeEntry->createInstr<BinaryOp>(InstrKind::Add, x, one);
    calleeEntry->createInstr<Return>(&sum);

    Function& caller = program->createFunction("caller");
    Parameter* y = caller.createParam("y");
    Constant* two = caller.createConstant(2, "2");
    BasicBlock* entry = caller.createBasicBlock("entry");
    auto& call = entry->createInstr<Call>(&callee, std::vector<Value*>{y});
    entry->createInstr<BinaryOp>(InstrKind::Mul, &call, two);

    StaticInlinerPass::Config config;
    config.runLocalOptimizations = false;
    ASSERT_TRUE(StaticInlinerPass::runOnFunction(caller, config));

    EXPECT_EQ(countCalls(caller), 0);
    EXPECT_EQ(countInstructions(caller, InstrKind::Add), 1);
    EXPECT_EQ(countInstructions(caller, InstrKind::Mul), 1);

    BasicBlock* continuation = findBlock(caller, "cont");
    ASSERT_NE(continuation, nullptr);
    ASSERT_FALSE(continuation->getInstructions().empty());
    auto* mul = dynamic_cast<BinaryOp*>(continuation->getInstructions().front().get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->getOperands()[0]->getValueKind(), Value::ValueKind::Instruction);
    EXPECT_EQ(dynamic_cast<Call*>(mul->getOperands()[0]), nullptr) << "call value should have been replaced";
}

TEST_F(OptimizationsTest, StaticInliningMultipleReturnsCreatesPhi) {
    Function& callee = program->createFunction("choose_value");
    Parameter* a = callee.createParam("a");
    Parameter* b = callee.createParam("b");
    Constant* one = callee.createConstant(1, "1");
    BasicBlock* start = callee.createBasicBlock("start");
    BasicBlock* branch = callee.createBasicBlock("branch");
    BasicBlock* left = callee.createBasicBlock("left");
    BasicBlock* right = callee.createBasicBlock("right");

    start->createInstr<Jump>("branch");
    branch->createInstr<BinaryOp>(InstrKind::Add, a, one);
    branch->createInstr<CondJump>("lecture_condition", "left", "right");
    left->createInstr<Return>(a);
    right->createInstr<Return>(b);

    CFGAnalysis::buildCFG(callee);

    Function& caller = program->createFunction("caller_with_branch");
    Constant* two = caller.createConstant(2, "2");
    Constant* three = caller.createConstant(3, "3");
    Constant* four = caller.createConstant(4, "4");
    BasicBlock* entry = caller.createBasicBlock("entry");
    auto& call = entry->createInstr<Call>(&callee, std::vector<Value*>{two, three});
    entry->createInstr<BinaryOp>(InstrKind::Mul, &call, four);

    StaticInlinerPass::Config config;
    config.runLocalOptimizations = false;
    ASSERT_TRUE(StaticInlinerPass::runOnFunction(caller, config));

    EXPECT_EQ(countCalls(caller), 0);
    EXPECT_EQ(countPhiInstructions(caller), 1);

    BasicBlock* continuation = findBlock(caller, "cont");
    ASSERT_NE(continuation, nullptr);
    ASSERT_GE(continuation->getInstructions().size(), 2U);
    auto* phi = dynamic_cast<Phi*>(continuation->getInstructions()[0].get());
    ASSERT_NE(phi, nullptr);
    EXPECT_EQ(phi->getIncoming().size(), 2U);

    auto* mul = dynamic_cast<BinaryOp*>(continuation->getInstructions()[1].get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->getOperands()[0], phi);
}
