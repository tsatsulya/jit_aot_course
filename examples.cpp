#include "program.h"
#include "loop_analysis.h"
#include "bin_ops.h"
#include "control_flow.h"

class GraphExamples {
public:
    static void createExample1(Program& program) {
        Function& func = program.createFunction("example1");

        BasicBlock* A = func.createBasicBlock("A");
        BasicBlock* B = func.createBasicBlock("B");
        BasicBlock* C = func.createBasicBlock("C");
        BasicBlock* D = func.createBasicBlock("D");
        BasicBlock* E = func.createBasicBlock("E");
        BasicBlock* F = func.createBasicBlock("F");
        BasicBlock* G = func.createBasicBlock("G");

        [[maybe_unused]] Constant* condTrue = func.createConstant(1, "true");
        [[maybe_unused]] Constant* condFalse = func.createConstant(0, "false");

        // Build CFG for Example 1:
        // A: B
        // B: C, F
        // C: D
        // D:
        // E: D
        // F: E, G
        // G: D

        A->createInstr<Jump>("B");

        B->createInstr<CondJump>("condition", "C", "F");

        C->createInstr<Jump>("D");

        D->createInstr<Return>();

        E->createInstr<Jump>("D");

        F->createInstr<CondJump>("condition", "E", "G");

        G->createInstr<Jump>("D");

        CFGAnalysis::buildCFG(func);
    }

    static void createExample2(Program& program) {
        Function& func = program.createFunction("example2");

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

        // Build CFG for Example 2:
        // A: B
        // B: C, J
        // C: D
        // D: E, C
        // E: F
        // F: G, E
        // G: H, I
        // H: B
        // I: K
        // J: C
        // K:

        A->createInstr<Jump>("B");
        B->createInstr<CondJump>("condition", "C", "J");
        C->createInstr<Jump>("D");
        D->createInstr<CondJump>("condition", "E", "C");
        E->createInstr<Jump>("F");
        F->createInstr<CondJump>("condition", "G", "E");
        G->createInstr<CondJump>("condition", "H", "I");
        H->createInstr<Jump>("B");
        I->createInstr<Jump>("K");
        J->createInstr<Jump>("C");
        K->createInstr<Return>();

        CFGAnalysis::buildCFG(func);
    }

    static void createExample3(Program& program) {
        Function& func = program.createFunction("example3");

        BasicBlock* A = func.createBasicBlock("A");
        BasicBlock* B = func.createBasicBlock("B");
        BasicBlock* C = func.createBasicBlock("C");
        BasicBlock* D = func.createBasicBlock("D");
        BasicBlock* E = func.createBasicBlock("E");
        BasicBlock* F = func.createBasicBlock("F");
        BasicBlock* G = func.createBasicBlock("G");
        BasicBlock* H = func.createBasicBlock("H");
        BasicBlock* I = func.createBasicBlock("I");

        // Build CFG for Example 3:
        // A: B
        // B: C, E
        // C: D
        // D: G
        // E: D, F
        // F: H, B
        // G: I, C
        // H: G, I
        // I:

        A->createInstr<Jump>("B");
        B->createInstr<CondJump>("condition", "C", "E");
        C->createInstr<Jump>("D");
        D->createInstr<Jump>("G");
        E->createInstr<CondJump>("condition", "D", "F");
        F->createInstr<CondJump>("condition", "H", "B");
        G->createInstr<CondJump>("condition", "I", "C");
        H->createInstr<CondJump>("condition", "G", "I");
        I->createInstr<Return>();

        CFGAnalysis::buildCFG(func);
    }
};

int main() {
    Program program;

    GraphExamples::createExample1(program);
    GraphExamples::createExample2(program);
    GraphExamples::createExample3(program);

    for (auto& func_ptr : program.getFunctions()) {
        auto dominators = CFGAnalysis::computeDominators(*func_ptr);

        std::cout << "\nDominators for " << func_ptr->getName() << ":" << std::endl;
        for (auto& [bb, dom_set] : dominators) {
            std::cout << bb->getName() << ": ";
            for (auto dom : dom_set) {
                std::cout << dom->getName() << " ";
            }
            std::cout << std::endl;
        }
    }

    return 0;
}