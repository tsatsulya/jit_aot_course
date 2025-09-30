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

    Parameter& n = f.createParam("n");
    Parameter& one = f.createParam("1");  // constant '1'

    BasicBlock& entry = f.createBasicBlock("entry");
    BasicBlock& loopCond = f.createBasicBlock("loopCond");
    BasicBlock& loopBody = f.createBasicBlock("loopBody");
    BasicBlock& afterLoop = f.createBasicBlock("afterLoop");

    entry.createInstr<Jump>(&loopCond);

    Phi& i_phi = loopCond.createInstr<Phi>();
    Phi& result_phi = loopCond.createInstr<Phi>();

    i_phi.addIncoming(&entry, &n);          // i = n
    result_phi.addIncoming(&entry, &one);   // result = 1

    // Cmp i > 1
    Cmp& cmp = loopCond.createInstr<Cmp>(CmpOp::Gt, &i_phi, &one);

    loopCond.createInstr<CondJump>(&cmp, &loopBody, &afterLoop);

    // result = result * i; then i = i - 1
    BinaryOp& mul = loopBody.createInstr<BinaryOp>(InstrKind::Mul, &result_phi, &i_phi);
    BinaryOp& sub = loopBody.createInstr<BinaryOp>(InstrKind::Sub, &i_phi, &one);

    loopBody.createInstr<Jump>(&loopCond);

    i_phi.addIncoming(&loopBody, &sub);
    result_phi.addIncoming(&loopBody, &mul);

    afterLoop.createInstr<Return>(&result_phi);

    prog.dump();

    return 0;
}


// Result:
//   Function factorial(%v0, %v1)
//   entry:                                                                                                                            tsatsulya@DESKTOP-EN6DV8O
//     jump loopCond
//   loopCond:
//     %v0 = phi [%n, entry], [%v5, loopBody]
//     %v1 = phi [%1, entry], [%v4, loopBody]
//     %v2 = cmp.gt %v0, %v1
//     cjump %v2, loopBody, afterLoop
//   loopBody:
//     %v4 = mul %v1, %v0
//     %v5 = sub %v0, %v1
//     jump loopCond
//   afterLoop:
//     return %v1