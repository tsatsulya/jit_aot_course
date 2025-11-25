#include <gtest/gtest.h>
#include "program.h"
#include "constant_folding.h"
#include "peephole_optimizer.h"
#include "bin_ops.h"

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
            if (auto constant = dynamic_cast<Constant*>(instr.get())) {
                if (constant->getValue() == value) {
                    return true;
                }
            }
        }
        return false;
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
    Constant* c1 = func.createConstant(1, "1");

    bb->createInstr<BinaryOp>(InstrKind::Mul, c4, c0);
    bb->createInstr<BinaryOp>(InstrKind::And, c4, c1);

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
    Constant* one = func.createConstant(1, "1");

    bb->createInstr<BinaryOp>(InstrKind::Mul, param, zero);
    bb->createInstr<BinaryOp>(InstrKind::And, param, one);
    bb->createInstr<BinaryOp>(InstrKind::Shr, param, zero);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 1);
    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 1);

    PeepholePass::runOnFunction(func);

    EXPECT_EQ(countInstructions(func, InstrKind::Mul), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::And), 0);
    EXPECT_EQ(countInstructions(func, InstrKind::Shr), 0);
}