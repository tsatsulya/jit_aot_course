#pragma once

#include "basic_block.h"
#include "function.h"
#include "loop_analysis.h"
#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class LinearOrder {
  public:
    static std::vector<BasicBlock *> compute(Function &function) {
        auto &basicBlocks = function.getBasicBlocks();
        if (basicBlocks.empty())
            return {};

        // find all natural loops
        auto loops = LoopAnalysis::findLoops(function);

        // collect all back edges
        std::set<std::pair<BasicBlock *, BasicBlock *>> backEdges;
        for (auto *loop : loops) {
            if (loop->latch && loop->header)
                backEdges.insert({loop->latch, loop->header});
        }

        // map each block to its smallest containing loop
        std::sort(loops.begin(), loops.end(),
                  [](LoopAnalysis::Loop *a, LoopAnalysis::Loop *b) {
                      return a->blocks.size() > b->blocks.size();
                  });
        std::unordered_map<BasicBlock *, LoopAnalysis::Loop *> blockToLoop;
        for (auto *loop : loops) {
            blockToLoop[loop->header] = loop;
            for (auto *bb : loop->blocks)
                blockToLoop[bb] = loop;
        }

        // Kahn's topological sort
        std::unordered_map<BasicBlock *, int> predCount;
        for (auto &bb : basicBlocks)
            predCount[bb.get()] = 0;
        for (auto &bb : basicBlocks) {
            for (auto *succ : bb->getSuccessors()) {
                if (!backEdges.count({bb.get(), succ}))
                    predCount[succ]++;
            }
        }

        std::vector<BasicBlock *> worklist;
        std::unordered_set<BasicBlock *> inWorklist;
        BasicBlock *entry = basicBlocks[0].get();
        worklist.push_back(entry);
        inWorklist.insert(entry);

        std::vector<BasicBlock *> order;
        order.reserve(basicBlocks.size());

        while (!worklist.empty()) {
            LoopAnalysis::Loop *currentLoop = nullptr;
            if (!order.empty()) {
                auto it = blockToLoop.find(order.back());
                if (it != blockToLoop.end())
                    currentLoop = it->second;
            }

            int selectedIdx = static_cast<int>(worklist.size()) - 1;
            if (currentLoop) {
                for (int i = static_cast<int>(worklist.size()) - 1; i >= 0;
                     --i) {
                    auto it = blockToLoop.find(worklist[i]);
                    if (it != blockToLoop.end() && it->second == currentLoop) {
                        selectedIdx = i;
                        break;
                    }
                }
            }

            BasicBlock *selected = worklist[selectedIdx];
            worklist.erase(worklist.begin() + selectedIdx);
            inWorklist.erase(selected);
            order.push_back(selected);

            for (auto *succ : selected->getSuccessors()) {
                if (backEdges.count({selected, succ}))
                    continue;
                if (--predCount[succ] == 0 && !inWorklist.count(succ)) {
                    worklist.push_back(succ);
                    inWorklist.insert(succ);
                }
            }
        }

        for (auto *loop : loops)
            delete loop;
        return order;
    }
};
