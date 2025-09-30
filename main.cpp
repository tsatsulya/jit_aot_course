#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "bin_ops.h"
#include "control_flow.h"
#include "instruction.h"
#include "program.h"

int main() {
    Program prog;
    Function& f = prog.createFunction("factorial");

    Parameter* n = f.createParam("n");
    static Constant one_const(1, "1");

    BasicBlock* entry = f.createBasicBlock("entry");
    BasicBlock* loopCond = f.createBasicBlock("loopCond");
    BasicBlock* loopBody = f.createBasicBlock("loopBody");
    BasicBlock* afterLoop = f.createBasicBlock("afterLoop");

    entry->createInstr<Jump>(loopCond);

    Phi& i_phi = loopCond->createInstr<Phi>();
    Phi& result_phi = loopCond->createInstr<Phi>();

    Cmp& cmp = loopCond->createInstr<Cmp>(CmpOp::Gt, &i_phi, &one_const);
    loopCond->createInstr<CondJump>(&cmp, loopBody, afterLoop);

    BinaryOp& mul = loopBody->createInstr<BinaryOp>(InstrKind::Mul, &result_phi, &i_phi);
    BinaryOp& sub = loopBody->createInstr<BinaryOp>(InstrKind::Sub, &i_phi, &one_const);
    loopBody->createInstr<Jump>(loopCond);

    i_phi.addIncoming(entry, n);
    result_phi.addIncoming(entry, &one_const);
    i_phi.addIncoming(loopBody, &sub);
    result_phi.addIncoming(loopBody, &mul);

    afterLoop->createInstr<Return>(&result_phi);

    prog.dump();
    return 0;
}

// Function factorial(%n)
// entry:
//   jump loopCond
// loopCond:
//   %v0 = phi [%n, entry], [%v1, loopBody]
//   %v2 = phi [1, entry], [%v3, loopBody]
//   %v4 = cmp.gt %v0, 1
//   cjump %v4, loopBody, afterLoop
// loopBody:
//   %v3 = mul %v2, %v0
//   %v1 = sub %v0, 1
//   jump loopCond
// afterLoop:
//   return %v2