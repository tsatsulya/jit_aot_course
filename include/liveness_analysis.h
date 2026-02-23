#pragma once

#include "basic_block.h"
#include "control_flow.h"
#include "function.h"
#include "instruction.h"
#include "linear_order.h"
#include "loop_analysis.h"
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct LiveRange {
    int from;
    int to;
    bool operator==(const LiveRange &o) const {
        return from == o.from && to == o.to;
    }
};

struct LifetimeInterval {
    Value *value = nullptr;
    std::vector<LiveRange> ranges;

    void addRange(int from, int to) {
        if (from >= to)
            return;
        ranges.push_back({from, to});
        mergeRanges();
    }

    void setFrom(int newFrom) {
        if (ranges.empty()) {
            ranges.push_back({newFrom, newFrom + 2});
            return;
        }
        ranges[0].from = newFrom;
        mergeRanges();
    }

    bool isLiveAt(int pos) const {
        for (const auto &r : ranges) {
            if (pos >= r.from && pos < r.to)
                return true;
            if (pos < r.from)
                break;
        }
        return false;
    }

    const std::vector<LiveRange> &getLiveRanges() const { return ranges; }

  private:
    void mergeRanges() {
        if (ranges.empty())
            return;
        std::sort(ranges.begin(), ranges.end(),
                  [](const LiveRange &a, const LiveRange &b) {
                      return a.from < b.from;
                  });
        std::vector<LiveRange> merged;
        merged.push_back(ranges[0]);
        for (size_t i = 1; i < ranges.size(); ++i) {
            auto &last = merged.back();
            if (ranges[i].from <= last.to) {
                last.to = std::max(last.to, ranges[i].to);
            } else {
                merged.push_back(ranges[i]);
            }
        }
        ranges = std::move(merged);
    }
};

class LivenessAnalysis {
  public:
    void build(Function &function) {
        intervals_.clear();
        instrToId_.clear();
        blockFrom_.clear();
        blockTo_.clear();
        liveIn_.clear();

        linearOrder_ = LinearOrder::compute(function);
        numberInstructions();

        auto loops = LoopAnalysis::findLoops(function);
        std::unordered_map<BasicBlock *, LoopAnalysis::Loop *> headerToLoop;
        for (auto *loop : loops) {
            headerToLoop[loop->header] = loop;
        }

        buildIntervals(headerToLoop);

        for (auto *loop : loops)
            delete loop;
    }

    const std::vector<BasicBlock *> &getLinearOrder() const {
        return linearOrder_;
    }

    const LifetimeInterval *getInterval(Value *v) const {
        auto it = intervals_.find(v);
        return (it != intervals_.end()) ? &it->second : nullptr;
    }

    std::vector<LiveRange> getLiveRanges(Value *v) const {
        auto it = intervals_.find(v);
        if (it == intervals_.end())
            return {};
        return it->second.getLiveRanges();
    }

    bool isLiveAt(Value *v, int pos) const {
        auto it = intervals_.find(v);
        if (it == intervals_.end())
            return false;
        return it->second.isLiveAt(pos);
    }

    int getInstructionId(Instruction *instr) const {
        auto it = instrToId_.find(instr);
        return (it != instrToId_.end()) ? it->second : -1;
    }

    int getBlockFrom(BasicBlock *bb) const {
        auto it = blockFrom_.find(bb);
        return (it != blockFrom_.end()) ? it->second : -1;
    }

    int getBlockTo(BasicBlock *bb) const {
        auto it = blockTo_.find(bb);
        return (it != blockTo_.end()) ? it->second : -1;
    }

  private:
    std::vector<BasicBlock *> linearOrder_;
    std::unordered_map<Value *, LifetimeInterval> intervals_;
    std::unordered_map<Instruction *, int> instrToId_;
    std::unordered_map<BasicBlock *, int> blockFrom_;
    std::unordered_map<BasicBlock *, int> blockTo_;
    std::unordered_map<BasicBlock *, std::unordered_set<Value *>> liveIn_;

    void numberInstructions() {
        int idx = 0;
        for (auto *bb : linearOrder_) {
            blockFrom_[bb] = 2 * idx;
            for (auto &instrPtr : bb->getInstructions()) {
                instrToId_[instrPtr.get()] = 2 * idx;
                ++idx;
            }
            blockTo_[bb] = 2 * idx;
        }
    }

    LifetimeInterval &getOrCreate(Value *v) {
        auto &iv = intervals_[v];
        iv.value = v;
        return iv;
    }

    static bool producesValue(Instruction *instr) {
        auto k = instr->getKind();
        return k != InstrKind::Jump && k != InstrKind::CondJump &&
               k != InstrKind::Return;
    }

    static bool isTracked(Value *v) {
        return v->getValueKind() != Value::ValueKind::Constant;
    }

    void
    buildIntervals(const std::unordered_map<BasicBlock *, LoopAnalysis::Loop *>
                       &headerToLoop) {
        for (int bi = static_cast<int>(linearOrder_.size()) - 1; bi >= 0;
             --bi) {
            BasicBlock *b = linearOrder_[bi];
            int bFrom = blockFrom_[b];
            int bTo = blockTo_[b];

            std::unordered_set<Value *> live;

            for (auto *succ : b->getSuccessors()) {
                auto it = liveIn_.find(succ);
                if (it != liveIn_.end()) {
                    live.insert(it->second.begin(), it->second.end());
                }
            }

            for (auto *succ : b->getSuccessors()) {
                for (auto &instrPtr : succ->getInstructions()) {
                    if (auto *phi = dynamic_cast<Phi *>(instrPtr.get())) {
                        for (auto &[pred, val] : phi->getIncoming()) {
                            if (pred == b && val && isTracked(val)) {
                                live.insert(val);
                            }
                        }
                    }
                }
            }

            for (auto *opd : live) {
                getOrCreate(opd).addRange(bFrom, bTo);
            }

            auto &instrs = b->getInstructions();
            for (int ii = static_cast<int>(instrs.size()) - 1; ii >= 0; --ii) {
                Instruction *op = instrs[ii].get();
                if (op->getKind() == InstrKind::Phi)
                    continue;

                int opId = instrToId_[op];

                if (producesValue(op)) {
                    getOrCreate(op).setFrom(opId);
                    live.erase(op);
                }

                for (auto *opd : op->getOperands()) {
                    if (!opd || !isTracked(opd))
                        continue;
                    getOrCreate(opd).addRange(bFrom, opId + 1);
                    live.insert(opd);
                }
            }

            for (auto &instrPtr : b->getInstructions()) {
                if (auto *phi = dynamic_cast<Phi *>(instrPtr.get())) {
                    live.erase(phi);
                }
            }

            auto loopIt = headerToLoop.find(b);
            if (loopIt != headerToLoop.end()) {
                LoopAnalysis::Loop *loop = loopIt->second;
                BasicBlock *latchBlock = loop->latch;
                if (latchBlock) {
                    int loopEndTo = blockTo_[latchBlock];
                    for (auto *opd : live) {
                        getOrCreate(opd).addRange(bFrom, loopEndTo);
                    }
                }
            }

            liveIn_[b] = std::move(live);
        }
    }
};
