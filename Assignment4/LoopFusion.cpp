#include "llvm/Transforms/Utils/LoopFusion.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

bool areLoopsAdjacent(Loop *Loop1, Loop *Loop2) {
    // Ensure neither loop is null
    if (!Loop1 || !Loop2) return false;
    // Get the header blocks of each loop
    BasicBlock *Header1 = Loop1->getHeader();
    BasicBlock *Header2 = Loop2->getHeader();
    // Check all exiting blocks of Loop1

    SmallVector<BasicBlock *, 4> ExitingBlocks;
    Loop1->getExitingBlocks(ExitingBlocks);
    for (BasicBlock *ExitingBlock : ExitingBlocks) {
    // Check if any of the successors of the exiting block is the header of Loop2
        for (BasicBlock *Successor : successors(ExitingBlock)) {
                if (Successor == Header2) {
                        return true;
                }
        }
    }
    // If no match was found, the loops are not adjacent
    return false;
}

PreservedAnalyses LoopFusion::run(Function &F,
FunctionAnalysisManager &AM) {
        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);

        for (auto &L : LI) {
               // Example: Iterate over all pairs of loops and check adjacency
               for (auto &L2 : LI) {
                   if (L != L2) { // Ensure we're not comparing the same loop
                       if (areLoopsAdjacent(L, L2)) {
                           errs() << "Loops are adjacent\n";
                       }
                   }
               }
           }
           return PreservedAnalyses::all();
}
