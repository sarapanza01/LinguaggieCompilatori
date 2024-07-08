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
// bool areLoopsAdjacent(Loop *Loop1, Loop *Loop2) {
//     if (!Loop1 || !Loop2) return false;
//     BasicBlock *Header1 = Loop1->getHeader();
//     BasicBlock *Header2 = Loop2->getHeader();

//     SmallVector<BasicBlock *, 4> ExitingBlocks;
//     Loop1->getExitingBlocks(ExitingBlocks);
//     for (BasicBlock *ExitingBlock : ExitingBlocks) {
//         for (BasicBlock *Successor : successors(ExitingBlock)) {
//             if (Successor == Header2) {
//                 return true;
//             }
//         }
//     }   
//     return false;
// }


bool areLoopsAdjacent(Loop *Loop1, Loop *Loop2) {

    BasicBlock *ExitingBlock = Loop1->getExitBlock(); 
    BasicBlock *PreHeader = Loop2->getLoopPreheader(); 
    BasicBlock *Header = Loop2->getHeader(); 

    //controllo per escludere un segm core fault
    if (!ExitingBlock || !PreHeader || !Header) {
        errs() << "One of the blocks is null\n";
        return false;
    }

    if(Loop1->isGuarded() && Loop2->isGuarded())
    {
        errs() << "I due loop sono guarded\n"; 

        for (BasicBlock *Successor : successors(ExitingBlock)) {
            if (Successor == Header) {
                errs() << "Il successore" << Successor << " è uguale all'header "<<Header<<"\n"; 
                return true;
            }
        }
    }
    else {
        errs() << "I due loop non sono guarded.\n"; 
        errs() << "L'exit block di L0 "<<ExitingBlock <<" deve essere il preheader di L1 "<<PreHeader<<".\n"; 

        if(ExitingBlock == PreHeader)
        {
            errs() << "Sono adiacenti\n"; 
            return true; 
        }
        else{
            errs() << "Provo a fare come nel codice di riferimento: \n"; 
            Instruction *terminator = ExitingBlock->getTerminator(); 
            if(!terminator)
            {
                errs() << "terminator non esiste: "<<terminator; 
                return false; 
            }
            errs() << "Terminator è: "<<terminator<<"\n"; 

            if (auto *BI = dyn_cast<BranchInst>(terminator)) {
                for (unsigned i = 0, e = BI->getNumSuccessors(); i != e; ++i) {
                    BasicBlock *successor = BI->getSuccessor(i);
                    errs() << "Successore: " << successor->getName() << " \n";
                    if (successor == Header || successor == PreHeader) {
                        errs() << "Successore trovato: " << successor->getName() << "\n";
                        return true;
                    }
                }
            } else if (auto *SI = dyn_cast<SwitchInst>(terminator)) {
                for (auto &Case : SI->cases()) {
                    BasicBlock *successor = Case.getCaseSuccessor();
                    errs() << "Successore: " << successor->getName() << " \n";
                    if (successor == Header || successor == PreHeader) {
                        errs() << "Successore trovato: " << successor->getName() << "\n";
                        return true;
                    }
                }
            } else if (auto *IBI = dyn_cast<IndirectBrInst>(terminator)) {
                for (unsigned i = 0, e = IBI->getNumDestinations(); i != e; ++i) {
                    BasicBlock *successor = IBI->getDestination(i);
                    errs() << "Successore: " << successor->getName() << " \n";
                    if (successor == Header || successor == PreHeader) {
                        errs() << "Successore trovato: " << successor->getName() << "\n";
                        return true;
                    }
                }
            } else if (auto *RI = dyn_cast<ReturnInst>(terminator)) {
                // Handle ReturnInst if applicable
                errs() << "ReturnInst found, no successors to check\n";
            } else if (auto *UI = dyn_cast<UnreachableInst>(terminator)) {
                // Handle UnreachableInst if applicable
                errs() << "UnreachableInst found, no successors to check\n";
            } else {
                errs() << "L'istruzione di terminazione non ha successori\n";
                return false;
            }

            // BasicBlock *successor = terminator->getSuccessor(0); 
            // errs() << "Successore: "<<successor<<" \n"; 
        }
    }

    errs()<<"Non sono adiacenti\n"; 
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
                        if(dep->isAnti()) {
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
    errs() << "Cerco di ottenere le var: "<<"\n"; 
    PHINode *IndVar0 = L0->getCanonicalInductionVariable();
    errs() << "Sono riuscito ad ottenere IndVar0: "<<IndVar0 <<"\n"; 
    PHINode *IndVar1 = L1->getCanonicalInductionVariable();
    errs() << "Sono riuscito ad ottenere IndVar0: "<<IndVar0 <<"\n"; 


    if (!IndVar0 || !IndVar1) return;

    // Popola la mappa delle sostituzioni
    errs() << "Popola la mappa delle sostituzioni: "<<"\n"; 
    IndVarMap[IndVar1] = IndVar0;

    // Sostituisce gli usi della variabile di induzione di L1 con quella di L0
    for (auto &BB : L1->blocks()) {
        for (auto &I : *BB) {
            for (unsigned int i = 0; i < I.getNumOperands(); ++i) {
                Value *Op = I.getOperand(i);
                errs() << "Provo a popolare le cose con op" << Op <<"\n"; 

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
    // LI.markAsRemoved(L1);
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

    for (auto iterL0 = LI.begin(); iterL0 != LI.end(); ++iterL0) {
        Loop *L0 = *iterL0;
        outs() << "Il loop fa " << getLoopIterations(SE, L0) << " iterazioni\n";
        auto iterL1 = iterL0;
        ++iterL1; // Inizia dal loop successivo a L0
        // for (; iterL1 != LI.end(); ++iterL1) {
        for (auto iterL1 = LI.begin(); iterL1 != LI.end(); ++iterL1) {
            Loop *L1 = *iterL1; 
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
