#include <gtest/gtest.h>
#include "program.h"
#include "cfg.h"
#include "linear_order.h"
#include "liveness_analysis.h"
#include "bin_ops.h"
#include "control_flow.h"
#include <vector>
#include <algorithm>

// collect block names from an ordered list
static std::vector<std::string> names(const std::vector<BasicBlock*>& order) {
    std::vector<std::string> n;
    for (auto* bb : order) n.push_back(bb->getName());
    return n;
}

// index of a block name in the linear order
static int indexOf(const std::vector<BasicBlock*>& order, const std::string& name) {
    for (int i = 0; i < (int)order.size(); ++i)
        if (order[i]->getName() == name) return i;
    return -1;
}

// Simple while loop with conditional exit
//
//   entry → loop_header ──(cond true)──→ loop_body → loop_header (back)
//             loop_header ──(cond false)──→ exit
//
//   entry:       v0 = param "x"
//                v1 = add v0, const_1
//                jump loop_header
//   loop_header: v2 = phi [v1, entry], [v3, loop_body]
//                condjump "cond" loop_body exit
//   loop_body:   v3 = mul v2, v2
//                jump loop_header           ← back edge
//   exit:        return v2
// ═══════════════════════════════════════════════════════════════════
TEST(LinearOrderTest, SimpleLoopLinearOrder) {
    Program prog;
    Function& func = prog.createFunction("simple_loop");

    BasicBlock* entry       = func.createBasicBlock("entry");
    BasicBlock* loop_header = func.createBasicBlock("loop_header");
    BasicBlock* loop_body   = func.createBasicBlock("loop_body");
    BasicBlock* exitBB      = func.createBasicBlock("exit");

    Parameter* param_x  = func.createParam("x");
    Constant*  const_1  = func.createConstant(1, "1");

    // entry
    BinaryOp& v1 = entry->createInstr<BinaryOp>(InstrKind::Add, param_x, const_1);
    entry->createInstr<Jump>("loop_header");

    // loop_header
    Phi& v2 = loop_header->createInstr<Phi>();
    loop_header->createInstr<CondJump>("cond", "loop_body", "exit");

    // loop_body
    BinaryOp& v3 = loop_body->createInstr<BinaryOp>(InstrKind::Mul, &v2, &v2);
    loop_body->createInstr<Jump>("loop_header");

    // exit
    exitBB->createInstr<Return>(&v2);

    // Wire phi operands (must be done after block pointers are stable)
    v2.addIncoming(entry, &v1);
    v2.addIncoming(loop_body, &v3);

    CFGAnalysis::buildCFG(func);

    auto order = LinearOrder::compute(func);
    ASSERT_EQ(order.size(), 4u);

    auto n = names(order);
    // entry must come first
    EXPECT_EQ(n[0], "entry");
    // loop blocks must be contiguous and before exit
    int hi = indexOf(order, "loop_header");
    int bi = indexOf(order, "loop_body");
    int ei = indexOf(order, "exit");
    EXPECT_NE(hi, -1);
    EXPECT_NE(bi, -1);
    EXPECT_NE(ei, -1);
    // loop_header before loop_body (header first)
    EXPECT_LT(hi, bi);
    // loop blocks contiguous: |hi - bi| == 1
    EXPECT_EQ(std::abs(hi - bi), 1);
    // exit comes after loop blocks
    EXPECT_GT(ei, bi);
    EXPECT_GT(ei, hi);
    // entry before loop_header (predecessor-before-block)
    EXPECT_LT(indexOf(order, "entry"), hi);
}

TEST(LivenessAnalysisTest, SimpleLoopLiveness) {
    Program prog;
    Function& func = prog.createFunction("simple_loop_live");

    BasicBlock* entry       = func.createBasicBlock("entry");
    BasicBlock* loop_header = func.createBasicBlock("loop_header");
    BasicBlock* loop_body   = func.createBasicBlock("loop_body");
    BasicBlock* exitBB      = func.createBasicBlock("exit");

    Parameter* param_x = func.createParam("x");
    Constant*  const_1 = func.createConstant(1, "1");

    BinaryOp& v1 = entry->createInstr<BinaryOp>(InstrKind::Add, param_x, const_1);
    entry->createInstr<Jump>("loop_header");

    Phi& v2 = loop_header->createInstr<Phi>();
    loop_header->createInstr<CondJump>("cond", "loop_body", "exit");

    BinaryOp& v3 = loop_body->createInstr<BinaryOp>(InstrKind::Mul, &v2, &v2);
    loop_body->createInstr<Jump>("loop_header");

    exitBB->createInstr<Return>(&v2);

    v2.addIncoming(entry, &v1);
    v2.addIncoming(loop_body, &v3);

    CFGAnalysis::buildCFG(func);

    LivenessAnalysis la;
    la.build(func);

    // param_x is used in entry's add → must be live in entry
    int entryFrom = la.getBlockFrom(entry);
    EXPECT_GE(entryFrom, 0);
    auto* paramRanges = la.getInterval(param_x);
    ASSERT_NE(paramRanges, nullptr);
    EXPECT_FALSE(paramRanges->getLiveRanges().empty());
    // param_x is live at the start of entry
    EXPECT_TRUE(la.isLiveAt(param_x, entryFrom));

    // v1 (add result) is a phi input consumed at the END of entry, not at loop_header.from.
    // The phi resolution happens at the entry→loop_header edge.
    EXPECT_TRUE(la.isLiveAt(&v1, la.getBlockTo(entry) - 1));

    // v2 (phi) is a loop-carried variable: live throughout the loop (loop_header + loop_body)
    int loopHeaderFrom = la.getBlockFrom(loop_header);
    int loopBodyFrom = la.getBlockFrom(loop_body);
    int loopBodyTo   = la.getBlockTo(loop_body);
    EXPECT_TRUE(la.isLiveAt(&v2, loopHeaderFrom));
    EXPECT_TRUE(la.isLiveAt(&v2, loopBodyFrom));
    // v2 is also used in exit (return v2)
    int exitFrom = la.getBlockFrom(exitBB);
    EXPECT_TRUE(la.isLiveAt(&v2, exitFrom));

    // v3 (mul result) is a phi input for loop_header: live in loop_body
    int v3Id = la.getInstructionId(&v3);
    EXPECT_GE(v3Id, 0);
    EXPECT_TRUE(la.isLiveAt(&v3, v3Id)); // live at its own definition
    // v3 must be live at end of loop_body (used as phi input for loop_header)
    EXPECT_TRUE(la.isLiveAt(&v3, loopBodyTo - 1));

    // Verify we have intervals for all computed values
    EXPECT_NE(la.getInterval(&v1), nullptr);
    EXPECT_NE(la.getInterval(&v2), nullptr);
    EXPECT_NE(la.getInterval(&v3), nullptr);
}

// Nested loops
//
//   entry → outer_header ──(false)──→ exit
//             outer_header ──(true)──→ inner_header
//             inner_header ──(true)──→ inner_body → inner_header (back)
//             inner_header ──(false)──→ outer_body → outer_header (back)
//
//   entry:        v0 = param "n"
//                 jump outer_header
//   outer_header: v1 = phi [v0, entry], [v4, outer_body]
//                 condjump "outer_cond" inner_header exit
//   inner_header: v2 = phi [v1, outer_header], [v3, inner_body]
//                 condjump "inner_cond" inner_body outer_body
//   inner_body:   v3 = add v2, const_1    ← uses v2
//                 jump inner_header       ← back edge (inner)
//   outer_body:   v4 = mul v1, v1         ← uses v1
//                 jump outer_header       ← back edge (outer)
//   exit:         return v1
// ═══════════════════════════════════════════════════════════════════
TEST(LinearOrderTest, NestedLoopsLinearOrder) {
    Program prog;
    Function& func = prog.createFunction("nested_loops");

    BasicBlock* entry        = func.createBasicBlock("entry");
    BasicBlock* outer_header = func.createBasicBlock("outer_header");
    BasicBlock* inner_header = func.createBasicBlock("inner_header");
    BasicBlock* inner_body   = func.createBasicBlock("inner_body");
    BasicBlock* outer_body   = func.createBasicBlock("outer_body");
    BasicBlock* exitBB       = func.createBasicBlock("exit");

    Parameter* param_n = func.createParam("n");
    Constant*  const_1 = func.createConstant(1, "1");

    // entry
    entry->createInstr<Jump>("outer_header");

    // outer_header
    Phi& v1 = outer_header->createInstr<Phi>();
    outer_header->createInstr<CondJump>("outer_cond", "inner_header", "exit");

    // inner_header
    Phi& v2 = inner_header->createInstr<Phi>();
    inner_header->createInstr<CondJump>("inner_cond", "inner_body", "outer_body");

    // inner_body
    BinaryOp& v3 = inner_body->createInstr<BinaryOp>(InstrKind::Add, &v2, const_1);
    inner_body->createInstr<Jump>("inner_header");

    // outer_body
    BinaryOp& v4 = outer_body->createInstr<BinaryOp>(InstrKind::Mul, &v1, &v1);
    outer_body->createInstr<Jump>("outer_header");

    // exit
    exitBB->createInstr<Return>(&v1);

    // Wire phis
    v1.addIncoming(entry, param_n);
    v1.addIncoming(outer_body, &v4);
    v2.addIncoming(outer_header, &v1);
    v2.addIncoming(inner_body, &v3);

    CFGAnalysis::buildCFG(func);

    auto order = LinearOrder::compute(func);
    ASSERT_EQ(order.size(), 6u);

    int entry_i  = indexOf(order, "entry");
    int outer_hi = indexOf(order, "outer_header");
    int inner_hi = indexOf(order, "inner_header");
    int inner_bi = indexOf(order, "inner_body");
    int outer_bi = indexOf(order, "outer_body");
    int exit_i   = indexOf(order, "exit");

    // All blocks found
    EXPECT_NE(entry_i,  -1);
    EXPECT_NE(outer_hi, -1);
    EXPECT_NE(inner_hi, -1);
    EXPECT_NE(inner_bi, -1);
    EXPECT_NE(outer_bi, -1);
    EXPECT_NE(exit_i,   -1);

    // entry first
    EXPECT_EQ(entry_i, 0);

    // outer_header before both inner blocks (dominator property)
    EXPECT_LT(outer_hi, inner_hi);
    EXPECT_LT(outer_hi, inner_bi);

    // inner loop blocks contiguous and before outer_body
    EXPECT_EQ(std::abs(inner_hi - inner_bi), 1);
    // inner loop blocks (inner_header, inner_body) are between outer_header and outer_body
    int inner_min = std::min(inner_hi, inner_bi);
    int inner_max = std::max(inner_hi, inner_bi);
    EXPECT_GT(inner_min, outer_hi);
    EXPECT_LT(inner_max, outer_bi);

    // exit comes after all loop blocks
    EXPECT_GT(exit_i, outer_bi);
}

TEST(LivenessAnalysisTest, NestedLoopsLiveness) {
    Program prog;
    Function& func = prog.createFunction("nested_loops_live");

    BasicBlock* entry        = func.createBasicBlock("entry");
    BasicBlock* outer_header = func.createBasicBlock("outer_header");
    BasicBlock* inner_header = func.createBasicBlock("inner_header");
    BasicBlock* inner_body   = func.createBasicBlock("inner_body");
    BasicBlock* outer_body   = func.createBasicBlock("outer_body");
    BasicBlock* exitBB       = func.createBasicBlock("exit");

    Parameter* param_n = func.createParam("n");
    Constant*  const_1 = func.createConstant(1, "1");

    entry->createInstr<Jump>("outer_header");

    Phi& v1 = outer_header->createInstr<Phi>();
    outer_header->createInstr<CondJump>("outer_cond", "inner_header", "exit");

    Phi& v2 = inner_header->createInstr<Phi>();
    inner_header->createInstr<CondJump>("inner_cond", "inner_body", "outer_body");

    BinaryOp& v3 = inner_body->createInstr<BinaryOp>(InstrKind::Add, &v2, const_1);
    inner_body->createInstr<Jump>("inner_header");

    BinaryOp& v4 = outer_body->createInstr<BinaryOp>(InstrKind::Mul, &v1, &v1);
    outer_body->createInstr<Jump>("outer_header");

    exitBB->createInstr<Return>(&v1);

    v1.addIncoming(entry, param_n);
    v1.addIncoming(outer_body, &v4);
    v2.addIncoming(outer_header, &v1);
    v2.addIncoming(inner_body, &v3);

    CFGAnalysis::buildCFG(func);

    LivenessAnalysis la;
    la.build(func);

    // v1 is an outer-loop variable: must be live throughout the outer loop
    // (outer_header, inner_header, inner_body, outer_body)
    int outer_h_from = la.getBlockFrom(outer_header);
    int inner_h_from = la.getBlockFrom(inner_header);
    int inner_b_from = la.getBlockFrom(inner_body);
    int outer_b_from = la.getBlockFrom(outer_body);
    int exit_from    = la.getBlockFrom(exitBB);

    EXPECT_TRUE(la.isLiveAt(&v1, outer_h_from));
    EXPECT_TRUE(la.isLiveAt(&v1, inner_h_from));
    EXPECT_TRUE(la.isLiveAt(&v1, inner_b_from));
    EXPECT_TRUE(la.isLiveAt(&v1, outer_b_from));
    // v1 is also used in exit (return)
    EXPECT_TRUE(la.isLiveAt(&v1, exit_from));

    // v2 is an inner-loop variable: live throughout the inner loop
    EXPECT_TRUE(la.isLiveAt(&v2, inner_h_from));
    EXPECT_TRUE(la.isLiveAt(&v2, inner_b_from));
    // v2 is the phi output of inner_header; it starts at inner_h_from
    // (it should NOT be live at outer_body since it's not used there)
    auto* iv2 = la.getInterval(&v2);
    ASSERT_NE(iv2, nullptr);
    EXPECT_FALSE(iv2->getLiveRanges().empty());

    // v3 is defined in inner_body, used as phi input of inner_header
    EXPECT_TRUE(la.isLiveAt(&v3, la.getBlockFrom(inner_body)));

    // param_n is live at start of entry
    EXPECT_TRUE(la.isLiveAt(param_n, la.getBlockFrom(entry)));
}

//  Loop with conditional (if-else) inside
//
//   entry → loop_header ──(false)──→ exit
//             loop_header ──(true)──→ if_block
//             if_block ──(true)──→ then_block ──→ merge → loop_header (back)
//             if_block ──(false)──→ else_block ──→ merge
//
//   entry:       v0 = param "a"
//                v1 = param "b"
//                jump loop_header
//   loop_header: v_acc = phi [v0, entry], [v_merge, merge]
//                condjump "loop_cond" if_block exit
//   if_block:    v_cmp = cmp.lt v_acc, const_10
//                condjump "inner_cond" then_block else_block
//   then_block:  v_then = add v_acc, v1     ← uses v_acc and v1
//                jump merge
//   else_block:  v_else = mul v_acc, const_2  ← uses v_acc, NOT v1
//                jump merge
//   merge:       v_merge = phi [v_then, then_block], [v_else, else_block]
//                jump loop_header           ← back edge
//   exit:        return v_acc
// ═══════════════════════════════════════════════════════════════════
TEST(LinearOrderTest, LoopWithConditionalLinearOrder) {
    Program prog;
    Function& func = prog.createFunction("loop_cond");

    BasicBlock* entry       = func.createBasicBlock("entry");
    BasicBlock* loop_header = func.createBasicBlock("loop_header");
    BasicBlock* if_block    = func.createBasicBlock("if_block");
    BasicBlock* then_block  = func.createBasicBlock("then_block");
    BasicBlock* else_block  = func.createBasicBlock("else_block");
    BasicBlock* merge       = func.createBasicBlock("merge");
    BasicBlock* exitBB      = func.createBasicBlock("exit");

    Parameter* param_a  = func.createParam("a");
    Parameter* param_b  = func.createParam("b");
    Constant*  const_10 = func.createConstant(10, "10");
    Constant*  const_2  = func.createConstant(2, "2");

    // entry
    entry->createInstr<Jump>("loop_header");

    // loop_header
    Phi& v_acc = loop_header->createInstr<Phi>();
    loop_header->createInstr<CondJump>("loop_cond", "if_block", "exit");

    // if_block
    if_block->createInstr<Cmp>(CmpOp::Lt, &v_acc, const_10);
    if_block->createInstr<CondJump>("inner_cond", "then_block", "else_block");

    // then_block
    BinaryOp& v_then = then_block->createInstr<BinaryOp>(InstrKind::Add, &v_acc, param_b);
    then_block->createInstr<Jump>("merge");

    // else_block
    BinaryOp& v_else = else_block->createInstr<BinaryOp>(InstrKind::Mul, &v_acc, const_2);
    else_block->createInstr<Jump>("merge");

    // merge
    Phi& v_merge = merge->createInstr<Phi>();
    merge->createInstr<Jump>("loop_header");

    // exit
    exitBB->createInstr<Return>(&v_acc);

    // Wire phis
    v_acc.addIncoming(entry, param_a);
    v_acc.addIncoming(merge, &v_merge);
    v_merge.addIncoming(then_block, &v_then);
    v_merge.addIncoming(else_block, &v_else);

    CFGAnalysis::buildCFG(func);

    auto order = LinearOrder::compute(func);
    ASSERT_EQ(order.size(), 7u);

    int entry_i   = indexOf(order, "entry");
    int lh_i      = indexOf(order, "loop_header");
    int if_i      = indexOf(order, "if_block");
    int then_i    = indexOf(order, "then_block");
    int else_i    = indexOf(order, "else_block");
    int merge_i   = indexOf(order, "merge");
    int exit_i    = indexOf(order, "exit");

    // All blocks present
    EXPECT_NE(entry_i, -1);
    EXPECT_NE(lh_i,    -1);
    EXPECT_NE(if_i,    -1);
    EXPECT_NE(then_i,  -1);
    EXPECT_NE(else_i,  -1);
    EXPECT_NE(merge_i, -1);
    EXPECT_NE(exit_i,  -1);

    // entry first
    EXPECT_EQ(entry_i, 0);

    // loop_header before all loop interior blocks
    EXPECT_LT(lh_i, if_i);
    EXPECT_LT(lh_i, then_i);
    EXPECT_LT(lh_i, else_i);
    EXPECT_LT(lh_i, merge_i);

    // All loop body blocks (if_block, then_block, else_block, merge) are before exit
    EXPECT_LT(if_i,    exit_i);
    EXPECT_LT(then_i,  exit_i);
    EXPECT_LT(else_i,  exit_i);
    EXPECT_LT(merge_i, exit_i);

    // merge (the latch / jump-back-to-header block) must be inside the loop region
    EXPECT_GT(merge_i, lh_i);
    EXPECT_LT(merge_i, exit_i);

    // if_block must precede both then and else (it dominates them)
    EXPECT_LT(if_i, then_i);
    EXPECT_LT(if_i, else_i);

    // merge must come after both branches
    EXPECT_GT(merge_i, then_i);
    EXPECT_GT(merge_i, else_i);
}

TEST(LivenessAnalysisTest, LoopWithConditionalLiveness) {
    Program prog;
    Function& func = prog.createFunction("loop_cond_live");

    BasicBlock* entry       = func.createBasicBlock("entry");
    BasicBlock* loop_header = func.createBasicBlock("loop_header");
    BasicBlock* if_block    = func.createBasicBlock("if_block");
    BasicBlock* then_block  = func.createBasicBlock("then_block");
    BasicBlock* else_block  = func.createBasicBlock("else_block");
    BasicBlock* merge       = func.createBasicBlock("merge");
    BasicBlock* exitBB      = func.createBasicBlock("exit");

    Parameter* param_a  = func.createParam("a");
    Parameter* param_b  = func.createParam("b");
    Constant*  const_10 = func.createConstant(10, "10");
    Constant*  const_2  = func.createConstant(2, "2");

    entry->createInstr<Jump>("loop_header");

    Phi& v_acc = loop_header->createInstr<Phi>();
    loop_header->createInstr<CondJump>("loop_cond", "if_block", "exit");

    Cmp& v_cmp = if_block->createInstr<Cmp>(CmpOp::Lt, &v_acc, const_10);
    if_block->createInstr<CondJump>("inner_cond", "then_block", "else_block");

    BinaryOp& v_then = then_block->createInstr<BinaryOp>(InstrKind::Add, &v_acc, param_b);
    then_block->createInstr<Jump>("merge");

    BinaryOp& v_else = else_block->createInstr<BinaryOp>(InstrKind::Mul, &v_acc, const_2);
    else_block->createInstr<Jump>("merge");

    Phi& v_merge = merge->createInstr<Phi>();
    merge->createInstr<Jump>("loop_header");

    exitBB->createInstr<Return>(&v_acc);

    v_acc.addIncoming(entry, param_a);
    v_acc.addIncoming(merge, &v_merge);
    v_merge.addIncoming(then_block, &v_then);
    v_merge.addIncoming(else_block, &v_else);

    CFGAnalysis::buildCFG(func);

    LivenessAnalysis la;
    la.build(func);

    // v_acc is live throughout the entire loop (used in if_block, then_block, else_block, exit)
    int lh_from    = la.getBlockFrom(loop_header);
    int if_from    = la.getBlockFrom(if_block);
    int then_from  = la.getBlockFrom(then_block);
    int else_from  = la.getBlockFrom(else_block);
    int merge_from = la.getBlockFrom(merge);
    int exit_from  = la.getBlockFrom(exitBB);

    EXPECT_TRUE(la.isLiveAt(&v_acc, lh_from));
    EXPECT_TRUE(la.isLiveAt(&v_acc, if_from));
    EXPECT_TRUE(la.isLiveAt(&v_acc, then_from));
    EXPECT_TRUE(la.isLiveAt(&v_acc, else_from));
    EXPECT_TRUE(la.isLiveAt(&v_acc, exit_from));

    EXPECT_TRUE(la.isLiveAt(param_b, then_from));
    EXPECT_TRUE(la.isLiveAt(param_b, else_from));
    EXPECT_FALSE(la.isLiveAt(&v_then, else_from));
    EXPECT_FALSE(la.isLiveAt(&v_else, then_from));

    int v_cmp_id = la.getInstructionId(&v_cmp);
    EXPECT_GE(v_cmp_id, 0);
    EXPECT_TRUE(la.isLiveAt(&v_cmp, v_cmp_id));

    int then_to = la.getBlockTo(then_block);
    EXPECT_TRUE(la.isLiveAt(&v_then, then_to - 1));

    int else_to = la.getBlockTo(else_block);
    EXPECT_TRUE(la.isLiveAt(&v_else, else_to - 1));

    EXPECT_TRUE(la.isLiveAt(&v_merge, merge_from));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
