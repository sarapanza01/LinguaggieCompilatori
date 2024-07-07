#include "llvm/Transforms/Utils/LoopFusion.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

// Funzione per verificare se due loop sono adiacenti
bool areLoopsAdjacent(Loop *Loop1, Loop *Loop2) {
    if (!Loop1 || !Loop2) return false;
    BasicBlock *Header1 = Loop1->getHeader();
    BasicBlock *Header2 = Loop2->getHeader();

    SmallVector<BasicBlock *, 4> ExitingBlocks;
    Loop1->getExitingBlocks(ExitingBlocks);
    for (BasicBlock *ExitingBlock : ExitingBlocks) {
        for (BasicBlock *Successor : successors(ExitingBlock)) {
            if (Successor == Header2) {
                return true;
            }
        }
    }
    return false;
}

// Funzione per ottenere il numero di iterazioni di un loop
int getLoopIterations(ScalarEvolution &SE, Loop *L) {
    int NumberOfIterations = SE.getSmallConstantTripCount(L);
    return NumberOfIterations + 1;
}

// Funzione per verificare se due loop sono equivalenti in termini di flusso di controllo
bool isControlFlowEquivalent(DominatorTree &DT, PostDominatorTree &PDT, Loop *L0, Loop *L1) {
    if (DT.dominates(L0->getHeader(), L1->getHeader()) && PDT.dominates(L1->getHeader(), L0->getHeader())) {
        return true;
    } else {
        return false;
    }
}

// Funzione per verificare se due loop hanno dipendenze valide
bool haveValidDep(DependenceInfo &DI, Loop* L0, Loop* L1) {
    for (BasicBlock *BB0 : L0->getBlocks()) {
        for (BasicBlock *BB1 : L1->getBlocks()) {
            for (Instruction &I0 : *BB0) {
                for (Instruction &I1 : *BB1) {
                    auto dep = DI.depends(&I0, &I1, true);
                    if (dep) {
                        outs() << "C'è dipendenza tra le istruzioni: " << I0 << " e " << I1 << "\n"; 
                        if(dep->isDirectionNegative()) {
                            errs() << "C'è dipendenza negativa tra le istruzioni (quindi non valida)"; 
                            return false; 
                        }
                        return true; 
                    }
                }
            }
        }
    }
    return false; 
}

// Funzione per modificare gli usi delle variabili di induzione
void replaceInductionVariables(Loop *L0, Loop *L1) {
    // Mappa per tracciare le sostituzioni
    DenseMap<Value*, Value*> IndVarMap;

    // Identifica la variabile di induzione in L0 e L1
    PHINode *IndVar0 = L0->getCanonicalInductionVariable();
    PHINode *IndVar1 = L1->getCanonicalInductionVariable();

    if (!IndVar0 || !IndVar1) return;

    // Popola la mappa delle sostituzioni
    IndVarMap[IndVar1] = IndVar0;

    // Sostituisce gli usi della variabile di induzione di L1 con quella di L0
    for (auto &BB : L1->blocks()) {
        for (auto &I : *BB) {
            for (unsigned int i = 0; i < I.getNumOperands(); ++i) {
                Value *Op = I.getOperand(i);
                if (IndVarMap.count(Op)) {
                    I.setOperand(i, IndVarMap[Op]);
                }
            }
        }
    }
}

// Funzione per fondere due loop
void fuseLoops(Function &F, Loop *L0, Loop *L1, LoopInfo &LI, DominatorTree &DT) {
    // Sostituisce le variabili di induzione
    replaceInductionVariables(L0, L1);

    // Unisce i corpi dei loop modificando il CFG
    BasicBlock *ExitingBlock = L0->getExitingBlock();
    BasicBlock *Header2 = L1->getHeader();
    if (ExitingBlock) {
        ExitingBlock->getTerminator()->replaceUsesOfWith(L1->getLoopPreheader(), Header2);
    }

    // Aggiorna il LoopInfo
    LI.removeBlock(L1->getHeader());
    LI.markAsRemoved(L1);
}

PreservedAnalyses LoopFusion::run(Function &F, FunctionAnalysisManager &AM) {
    // Per il punto 1
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    // Per il punto 2
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
    // Per il punto 3
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
    PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
    // Per il punto 4
    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

    for (auto &L0 : LI) {
        // Controlla il numero di iterazioni
        outs() << "Il loop fa " << getLoopIterations(SE, L0) << " iterazioni\n";
        for (auto &L1 : LI) {
            if (L0 != L1) {
                outs() << "Confronto tra due loop: " << L0 << " e " << L1 << "\n"; 
                if (areLoopsAdjacent(L0, L1)) {
                    errs() << "I loop sono adiacenti\n";
                    if (getLoopIterations(SE, L0) == getLoopIterations(SE, L1)) {
                        if (isControlFlowEquivalent(DT, PDT, L0, L1)) {
                            outs() << "I loop sono entrambi eseguiti\n";
                            if (haveValidDep(DI, L0, L1)) {
                                outs() << "Hanno dipendenze valide i loop " << L0 << " e " << L1 << "\n";
                                fuseLoops(F, L0, L1, LI, DT);
                            }
                        } else {
                            outs() << "I loop non sono equivalenti in termini di flusso di controllo\n";
                        }
                    }
                }
            }
        }
    }
    return PreservedAnalyses::all();
}
