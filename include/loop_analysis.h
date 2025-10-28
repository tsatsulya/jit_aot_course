#pragma once

#include "basic_block.h"
#include "cfg.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stack>
#include <algorithm>

class LoopAnalysis {
public:
    struct Loop {
        BasicBlock* header;
        BasicBlock* latch;
        std::unordered_set<BasicBlock*> blocks;
        std::unordered_set<BasicBlock*> exits;
        std::vector<Loop*> subLoops;
        Loop* parent;

        Loop(BasicBlock* hdr, BasicBlock* lch = nullptr, Loop* p = nullptr)
            : header(hdr), latch(lch), parent(p) {}

        void addBlock(BasicBlock* bb) { blocks.insert(bb); }
        void addExit(BasicBlock* bb) { exits.insert(bb); }
        void addSubLoop(Loop* loop) { subLoops.push_back(loop); }

        bool contains(BasicBlock* bb) const {
            return blocks.count(bb) || bb == header;
        }

        int getDepth() const {
            int depth = 0;
            Loop* current = parent;
            while (current) {
                depth++;
                current = current->parent;
            }
            return depth;
        }
    };

    static std::vector<Loop*> findLoops(Function& function) {
        std::vector<Loop*> loops;
        auto dominators = CFGAnalysis::computeDominators(function);
        auto& basicBlocks = function.getBasicBlocks();

        for (auto& bb : basicBlocks) {
            for (auto succ : bb->getSuccessors()) {
                if (dominators[bb.get()].count(succ)) {
                    Loop* loop = discoverLoop(bb.get(), succ, dominators);
                    if (loop) {
                        loops.push_back(loop);
                    }
                }
            }
        }

        buildLoopHierarchy(loops);
        return loops;
    }

    static void analyzeLoopNesting(Function& function) {
        auto loops = findLoops(function);

        std::cout << "Loop Analysis for Function: " << function.getName() << "\n";
        std::cout << "========================================\n";

        if (loops.empty()) {
            std::cout << "No loops found.\n";
            return;
        }

        for (auto loop : loops) {
            printLoopInfo(loop, 0);
        }

        for (auto loop : loops) {
            delete loop;
        }
    }

    static std::unordered_set<BasicBlock*> getLoopExits(Loop* loop) {
        std::unordered_set<BasicBlock*> exits;

        for (auto bb : loop->blocks) {
            for (auto succ : bb->getSuccessors()) {
                if (!loop->contains(succ)) {
                    exits.insert(bb);
                }
            }
        }

        for (auto succ : loop->header->getSuccessors()) {
            if (!loop->contains(succ)) {
                exits.insert(loop->header);
            }
        }

        return exits;
    }

    static bool isInnermostLoop(Loop* loop) {
        return loop->subLoops.empty();
    }

private:
    static Loop* discoverLoop(BasicBlock* latch, BasicBlock* header,
                            const std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& /* dominators */) {
        Loop* loop = new Loop(header, latch);
        loop->addBlock(header);

        std::stack<BasicBlock*> worklist;
        worklist.push(latch);

        while (!worklist.empty()) {
            BasicBlock* current = worklist.top();
            worklist.pop();

            if (loop->blocks.count(current)) {
                continue;
            }

            loop->addBlock(current);

            for (auto pred : current->getPredecessors()) {
                if (!loop->blocks.count(pred)) {
                    worklist.push(pred);
                }
            }
        }

        loop->exits = getLoopExits(loop);

        return loop;
    }

    static void buildLoopHierarchy(std::vector<Loop*>& loops) {
        std::sort(loops.begin(), loops.end(), [](Loop* a, Loop* b) {
            return a->blocks.size() > b->blocks.size();
        });

        for (size_t i = 0; i < loops.size(); ++i) {
            for (size_t j = i + 1; j < loops.size(); ++j) {
                if (loops[i]->contains(loops[j]->header)) {
                    loops[j]->parent = loops[i];
                    loops[i]->addSubLoop(loops[j]);
                }
            }
        }
    }

    static void printLoopInfo(Loop* loop, int indent) {
        std::string indentStr(indent * 2, ' ');

        std::cout << indentStr << "Loop (Depth: " << loop->getDepth() << "):\n";
        std::cout << indentStr << "  Header: " << loop->header->getName() << "\n";
        if (loop->latch) {
            std::cout << indentStr << "  Latch: " << loop->latch->getName() << "\n";
        }

        std::cout << indentStr << "  Blocks: ";
        for (auto bb : loop->blocks) {
            std::cout << bb->getName() << " ";
        }
        std::cout << "\n";

        if (!loop->exits.empty()) {
            std::cout << indentStr << "  Exit blocks: ";
            for (auto exit : loop->exits) {
                std::cout << exit->getName() << " ";
            }
            std::cout << "\n";
        }

        std::cout << indentStr << "  Innermost: " << (isInnermostLoop(loop) ? "Yes" : "No") << "\n";
        std::cout << "\n";

        for (auto subLoop : loop->subLoops) {
            printLoopInfo(subLoop, indent + 1);
        }
    }
};
