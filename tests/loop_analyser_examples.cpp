#include "loop_analysis.h"
#include "program.h"
#include <cassert>
#include <iostream>

class LoopAnalysisTest {
public:
    static void runAllTests() {
        testGraph1();
        testGraph2();
        testGraph3();
        testGraph4();
        testGraph5();

        std::cout << "All tests passed!\n";
    }

private:
    static void testGraph1() {
        std::cout << "Test 1:\n";
        std::cout << "CFG: A: B, B: C, D, C: , D: E, E: B\n";

        Program program;
        Function& func = program.createFunction("test1");

        BasicBlock* A = func.createBasicBlock("A");
        BasicBlock* B = func.createBasicBlock("B");
        BasicBlock* C = func.createBasicBlock("C");
        BasicBlock* D = func.createBasicBlock("D");
        BasicBlock* E = func.createBasicBlock("E");

        A->createInstr<Jump>("B");
        B->createInstr<CondJump>("cond", "C", "D");
        C->createInstr<Return>();
        D->createInstr<Jump>("E");
        E->createInstr<Jump>("B");

        CFGAnalysis::buildCFG(func);

        auto loops = LoopAnalysis::findLoops(func);

        assert(loops.size() == 1);

        auto* loop = loops[0];
        assert(loop->header->getName() == "B");
        assert(loop->latch->getName() == "E");
        assert(loop->blocks.size() == 3);
        assert(loop->contains(B));
        assert(loop->contains(D));
        assert(loop->contains(E));
        assert(!loop->contains(A));
        assert(!loop->contains(C));

        auto exits = LoopAnalysis::getLoopExits(loop);
        assert(exits.size() == 1);
        assert(exits.count(B));

        std::cout << "Test 1 passed: Found loop B->D->E->B\n\n";
        LoopAnalysis::analyzeLoopNesting(func);

        for (auto l : loops) delete l;
    }

    static void testGraph2() {
        std::cout << "Test 2:\n";
        std::cout << "CFG: A: B, B: C, C: D, F, D: E, F, E: B, F: \n";

        Program program;
        Function& func = program.createFunction("test2");

        BasicBlock* A = func.createBasicBlock("A");
        BasicBlock* B = func.createBasicBlock("B");
        BasicBlock* C = func.createBasicBlock("C");
        BasicBlock* D = func.createBasicBlock("D");
        BasicBlock* E = func.createBasicBlock("E");
        BasicBlock* F = func.createBasicBlock("F");

        A->createInstr<Jump>("B");
        B->createInstr<Jump>("C");
        C->createInstr<CondJump>("cond1", "D", "F");
        D->createInstr<CondJump>("cond2", "E", "F");
        E->createInstr<Jump>("B");
        F->createInstr<Return>();

        CFGAnalysis::buildCFG(func);

        auto loops = LoopAnalysis::findLoops(func);

        assert(loops.size() == 1 && "Should find exactly one loop");

        auto* loop = loops[0];
        assert(loop->header->getName() == "B");
        assert(loop->latch->getName() == "E");

        assert(loop->blocks.size() == 4);
        assert(loop->contains(B));
        assert(loop->contains(C));
        assert(loop->contains(D));
        assert(loop->contains(E));
        assert(!loop->contains(A));
        assert(!loop->contains(F));

        auto exits = LoopAnalysis::getLoopExits(loop);
        assert(exits.size() == 2);
        assert(exits.count(C));
        assert(exits.count(D));

        std::cout << "Test 2 passed: Found loop B->C->D->E->B\n\n";
        LoopAnalysis::analyzeLoopNesting(func);

        for (auto l : loops) delete l;
    }

    static void testGraph3() {
        std::cout << "Test 3:\n";
        std::cout << "CFG: A: B, B: C, D, C: E, F, D: F, E: , F: G, G: B, H, H: A\n";

        Program program;
        Function& func = program.createFunction("test3");

        BasicBlock* A = func.createBasicBlock("A");
        BasicBlock* B = func.createBasicBlock("B");
        BasicBlock* C = func.createBasicBlock("C");
        BasicBlock* D = func.createBasicBlock("D");
        BasicBlock* E = func.createBasicBlock("E");
        BasicBlock* F = func.createBasicBlock("F");
        BasicBlock* G = func.createBasicBlock("G");
        BasicBlock* H = func.createBasicBlock("H");

        A->createInstr<Jump>("B");
        B->createInstr<CondJump>("cond1", "C", "D");
        C->createInstr<CondJump>("cond2", "E", "F");
        D->createInstr<Jump>("F");
        E->createInstr<Return>();
        F->createInstr<Jump>("G");
        G->createInstr<CondJump>("cond3", "B", "H");
        H->createInstr<Jump>("A");

        CFGAnalysis::buildCFG(func);

        auto loops = LoopAnalysis::findLoops(func);

        assert(loops.size() >= 1);

        LoopAnalysis::Loop* outerLoop = nullptr;
        for (auto loop : loops) {
            if (loop->header->getName() == "A") {
                outerLoop = loop;
                break;
            }
        }

        assert(outerLoop != nullptr);
        assert(outerLoop->latch->getName() == "H");

        assert(outerLoop->contains(A));
        assert(outerLoop->contains(B));
        assert(outerLoop->contains(C));
        assert(outerLoop->contains(D));
        assert(outerLoop->contains(F));
        assert(outerLoop->contains(G));
        assert(outerLoop->contains(H));
        assert(!outerLoop->contains(E));

        LoopAnalysis::Loop* innerLoop = nullptr;
        for (auto loop : loops) {
            if (loop->header->getName() == "B" && loop != outerLoop) {
                innerLoop = loop;
                break;
            }
        }

        assert(outerLoop != nullptr);
        assert(innerLoop->latch->getName() == "G");

        assert(innerLoop->contains(B));
        assert(innerLoop->contains(C));
        assert(innerLoop->contains(D));
        assert(innerLoop->contains(F));
        assert(innerLoop->contains(G));
        assert(!innerLoop->contains(A));
        assert(!innerLoop->contains(E));
        assert(!innerLoop->contains(H));

        assert(innerLoop->parent == outerLoop);

        std::cout << "Test 3 passed\n";
        LoopAnalysis::analyzeLoopNesting(func);

        // std::cout << "  Outer loop: A->...->H->A\n";
        // if (innerLoop) {
        //     std::cout << "  Inner loop: B->...->G->B (nested)\n";
        // }
        std::cout << "\n";

        for (auto l : loops) delete l;
    }

    static void testGraph4() {
        std::cout << "Test 4:\n";
        std::cout << "CFG: A: B, B: C, F, C: D, D: , E: D, F: E, G, G: D\n";

        Program program;
        Function& func = program.createFunction("test4");

        BasicBlock* A = func.createBasicBlock("A");
        BasicBlock* B = func.createBasicBlock("B");
        BasicBlock* C = func.createBasicBlock("C");
        BasicBlock* D = func.createBasicBlock("D");
        BasicBlock* E = func.createBasicBlock("E");
        BasicBlock* F = func.createBasicBlock("F");
        BasicBlock* G = func.createBasicBlock("G");

        A->createInstr<Jump>("B");
        B->createInstr<CondJump>("cond1", "C", "F");
        C->createInstr<Jump>("D");
        D->createInstr<Return>();
        E->createInstr<Jump>("D");
        F->createInstr<CondJump>("cond2", "E", "G");
        G->createInstr<Jump>("D");

        CFGAnalysis::buildCFG(func);

        auto loops = LoopAnalysis::findLoops(func);

        assert(loops.size() == 0);

        std::cout << "Test 4 passed\n";
        LoopAnalysis::analyzeLoopNesting(func);

        for (auto l : loops) delete l;
    }

    static void testGraph5() {
        std::cout << "Test 5:\n";
        std::cout << "CFG: A: B, B: C, J, C: D, D: E, C, E: F, F: E, G, G: H, I, H: B, I: K, J: C, K: \n";

        Program program;
        Function& func = program.createFunction("test5");

        BasicBlock* A = func.createBasicBlock("A");
        BasicBlock* B = func.createBasicBlock("B");
        BasicBlock* C = func.createBasicBlock("C");
        BasicBlock* D = func.createBasicBlock("D");
        BasicBlock* E = func.createBasicBlock("E");
        BasicBlock* F = func.createBasicBlock("F");
        BasicBlock* G = func.createBasicBlock("G");
        BasicBlock* H = func.createBasicBlock("H");
        BasicBlock* I = func.createBasicBlock("I");
        BasicBlock* J = func.createBasicBlock("J");
        BasicBlock* K = func.createBasicBlock("K");

        A->createInstr<Jump>("B");
        B->createInstr<CondJump>("cond1", "C", "J");
        C->createInstr<Jump>("D");
        D->createInstr<CondJump>("cond2", "E", "C");
        E->createInstr<Jump>("F");
        F->createInstr<CondJump>("cond3", "E", "G");
        G->createInstr<CondJump>("cond4", "H", "I");
        H->createInstr<Jump>("B");
        I->createInstr<Jump>("K");
        J->createInstr<Jump>("C");
        K->createInstr<Return>();

        CFGAnalysis::buildCFG(func);

        auto loops = LoopAnalysis::findLoops(func);

        assert(loops.size() >= 2);

        LoopAnalysis::Loop* outerLoop = nullptr;
        for (auto loop : loops) {
            if (loop->header->getName() == "B") {
                outerLoop = loop;
                break;
            }
        }

        assert(outerLoop != nullptr);
        assert(outerLoop->latch->getName() == "H");

        assert(outerLoop->contains(B));
        assert(outerLoop->contains(C));
        assert(outerLoop->contains(D));
        assert(outerLoop->contains(E));
        assert(outerLoop->contains(F));
        assert(outerLoop->contains(G));
        assert(outerLoop->contains(H));
        assert(outerLoop->contains(J));

        LoopAnalysis::Loop* innerLoopCD = nullptr;
        for (auto loop : loops) {
            if (loop->header->getName() == "C" && loop != outerLoop) {
                innerLoopCD = loop;
                break;
            }
        }

        if (innerLoopCD != nullptr) {
            assert(innerLoopCD->latch->getName() == "D");
            assert(innerLoopCD->contains(C));
            assert(innerLoopCD->contains(D));
            assert(innerLoopCD->parent == outerLoop);
        }
        LoopAnalysis::Loop* selfLoopEF = nullptr;
        for (auto loop : loops) {
            if (loop->header->getName() == "E" && loop->latch->getName() == "F") {
                selfLoopEF = loop;
                break;
            }
        }

        if (selfLoopEF != nullptr) {
            assert(selfLoopEF->contains(E));
            assert(selfLoopEF->contains(F));
            assert(selfLoopEF->blocks.size() == 2);
            assert(selfLoopEF->parent == outerLoop);
        }

        assert(!outerLoop->contains(A));
        assert(!outerLoop->contains(I));
        assert(!outerLoop->contains(K));

        std::cout << "Test 5 passed:\n";
        // std::cout << "  Outer loop: B->...->H->B\n";
        LoopAnalysis::analyzeLoopNesting(func);
        // std::cout << "  Inner loop: C->D->C\n";
        // LoopAnalysis::printLoopInfo(innerLoopCD, 1);
        // std::cout << "  Self-loop: E->F->E\n";
        // LoopAnalysis::printLoopInfo(selfLoopEF, 1);
        std::cout << "\n";

        for (auto l : loops) delete l;
    }

};

int main() {
    LoopAnalysisTest::runAllTests();
    return 0;
}
