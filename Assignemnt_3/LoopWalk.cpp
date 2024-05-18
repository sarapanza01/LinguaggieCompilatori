#include "llvm/Transforms/Utils/LoopWalk.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/GenericLoopInfo.h"

using namespace llvm;
bool myIsLoopInvariant(Instruction &Inst, Loop &L) {
    for (auto *Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter) {
        Value *Operand = Iter->get();
        // Controllo che non sia un PHINode
        if (isa<PHINode>(Inst) || isa<BranchInst>(Inst)) {
            return false;
        }
        // Controllo che non sia una costante
        if (Instruction *arg = dyn_cast<Instruction>(Operand)) {
            // Controllo che la variabile sia dichiarata nel loop
            if (L.contains(arg)) {
                // In questo modo controllo iterativamente se
                // tutti gli operandi di una funzione sono
                // o meno loop invariant
                if (!myIsLoopInvariant(*arg, L)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool dominatesExits(Instruction *Inst, DominatorTree &DT, Loop &L) {
    // vettore in cui mi segno le uscite
    std::vector<BasicBlock*> exits;
    for (auto *BI : L.getBlocks()) {
        if (BI != L.getHeader() && L.isLoopExiting(BI)) {
            exits.push_back(BI);
        }
    }
                                          
 // Scorro le uscite e verifico se l’istruzione appartiene
    // ad una di queste
    for (auto *exit : exits) {
        if (!DT.dominates(Inst->getParent(), exit)) {
            return false;
        }
    }
    return true;
}

bool dominatesUseBlocks(Instruction *Inst, DominatorTree &DT) {
    for (auto Iter = Inst->user_begin(); Iter != Inst->user_end(); ++Iter) {
        if (!DT.dominates(Inst, dyn_cast<Instruction>(*Iter))) {
            return false;
        }
    }
    return true;
}

bool runOnLoop(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LAR, LPMUpdater &LU) {
    // Se il loop non è nella forma semplificata è inutile continuare
    if (!L.isLoopSimplifyForm()) {
        outs() << "Il loop non è in forma normalizzata\n";
        return false;
    }
    // Creo il dominance tree
    DominatorTree &DT = LAR.DT;
    // Creo un vettore per le istruzioni che devo spostare
    std::vector<Instruction*> InstToMove;
    // Creo un vettore per le istruzioni Loop Invariant
    std::vector<Instruction*> InstLI;

    // Itero sui BasicBlock del loop
    int i = 1; // Per contare I blocchi
    for (auto BI = L.block_begin(); BI != L.block_end(); ++BI) {
        BasicBlock *BB = *BI;
        outs() << "Blocco" << i << "\n";
        for (auto &Inst : *BB) {
            if (myIsLoopInvariant(Inst, L)) {
                outs() << Inst.getOpcodeName() << " é loop invariant\n";
                                                                           
 InstLI.push_back(&Inst);
            }
        }
        i++;
    }

    // Ora scrollo le istruzioni che si possono spostare e
    // controllo che siano in blocchi che dominano tutte le
    // uscite del loop e che dominano tutti I blocchi del loop
    // che usano la variabile
    for (auto *inst : InstLI) {
        if (dominatesExits(inst, DT, L) && dominatesUseBlocks(inst, DT)) {
            InstToMove.push_back(inst);
        }
    }

    // Ora devo spostare queste istruzioni nel preheader
    BasicBlock *preHeader = L.getLoopPreheader();
    if (!preHeader) {
        outs() << "Preheader non trovato\n";
        return false;
    }
    for (auto inst : InstToMove) {
        outs() << *inst << " è da spostare\n";
        inst->moveBefore(&preHeader->back());
    }
    return true;
}

PreservedAnalyses LoopWalk::run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LAR, LPMUpdater &LU) {
    if (runOnLoop(L, LAM, LAR, LU)) {
        return PreservedAnalyses::all();
    } else {
        return PreservedAnalyses::none();
    }
                                                  
}
