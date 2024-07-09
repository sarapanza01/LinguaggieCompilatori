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

#include "llvm/Transforms/Utils/BasicBlockUtils.h"


using namespace llvm;


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
            //codice di rif: https://sridhargopinath.in/files/loop-fusion.pdf
            errs() << "Provo a fare come nel codice di riferimento: \n"; 
            Instruction *terminator = ExitingBlock->getTerminator(); 
            if(!terminator)
            {
                errs() << "terminator non esiste: "<<terminator; 
                return false; 
            }
            errs() << "Terminator è: "<<terminator<<"\n"; 

            if (auto *BI = dyn_cast<BranchInst>(terminator)) {
                errs() << "E' una branchInst\n"; 
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
                errs() << "E' una SwitchInst\n"; 
                    BasicBlock *successor = Case.getCaseSuccessor();
                    errs() << "Successore: " << successor->getName() << " \n";
                    if (successor == Header || successor == PreHeader) {
                        errs() << "Successore trovato: " << successor->getName() << "\n";
                        return true;
                    }
                }
            } else if (auto *IBI = dyn_cast<IndirectBrInst>(terminator)) {
                errs() << "E' una IndirectBrInst\n"; 
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

void fuseLoops(Function &F, Loop *L0, Loop *L1, LoopInfo &LI, DominatorTree &DT) {
    PHINode *IndVar0 = L0->getCanonicalInductionVariable();
    PHINode *IndVar1 = L1->getCanonicalInductionVariable();

    IndVar1->replaceAllUsesWith(IndVar0); 

    BasicBlock *HeaderL0 = L0->getHeader();
    BasicBlock *HeaderL1 = L1->getHeader();
    BasicBlock *BodyL0 = L0->getHeader()->getNextNode();
    BasicBlock *BodyL1 = L1->getHeader()->getNextNode();
    BasicBlock *LatchL0 = L0->getLoopLatch();
    BasicBlock *LatchL1 = L1->getLoopLatch();
    BasicBlock *ExitL0 = L0->getExitBlock();
    BasicBlock *ExitL1 = L1->getExitBlock();
    BasicBlock *PreHeader0 = L0->getLoopPreheader(); 
    BasicBlock *PreHeader1 = L1->getLoopPreheader(); 
    
    // PreHeader1->replaceSuccessorsPhiUsesWith(PreHeader0);
    // LatchL0->replaceSuccessorsPhiUsesWith(LatchL1);

    if (!HeaderL0 || !HeaderL1 || !BodyL0 || !BodyL1 || !LatchL0 || !LatchL1 || !ExitL1) {
    errs() << "Errore: Blocchi di base non validi.\n";
    return;
    }

    errs()<<"Collega il corpo di L0 al corpo di L1\n";
    Instruction *TerminatorBodyL0 = BodyL0->getTerminator();
    TerminatorBodyL0->setSuccessor(0, BodyL1);

    errs()<<"Collega l'header di L0 all'uscita di L1\n";
    Instruction *TerminatorHeaderL0 = L0->getHeader()->getTerminator();
    for (unsigned i = 0; i < TerminatorHeaderL0->getNumSuccessors(); ++i) {
        if (TerminatorHeaderL0->getSuccessor(i) == PreHeader1) {
            TerminatorHeaderL0->setSuccessor(i, ExitL1);
            IndVar0->replaceIncomingBlockWith(PreHeader1, ExitL1); 
            break;
        }
    }

    errs()<<"Collega il corpo di L1 al latch di L0\n";
    Instruction *TerminatorBodyL1 = BodyL1->getTerminator();
    TerminatorBodyL1->setSuccessor(0, LatchL0);


    errs()<<"L'header di loop1 punta al latch di loop1\n"; 
    for (unsigned i = 0; i < HeaderL1->getTerminator()->getNumSuccessors(); ++i) {
        if (HeaderL1->getTerminator()->getSuccessor(i) == BodyL1) {
            HeaderL1->getTerminator()->setSuccessor(i, LatchL1);
        }
    }


    // Aggiorna LoopInfo e DominatorTree
    errs()<<"Aggiorna LoopInfo e DominatorTree\n";
    LI.removeBlock(HeaderL1);
    if (auto *ParentLoop = L0->getParentLoop()) {
        ParentLoop->removeBlockFromLoop(HeaderL1);
    } else {
        LI.removeBlock(HeaderL1);
    }

    DT.eraseNode(HeaderL1);
    // errs()<<"SetSuccessorOf L0\n";
    // b1->getTerminator()->setSuccessor(0, L1->getHeader()); 
    // auto *BI = dyn_cast<BranchInst>(terminator); 
    // BI->setSuccessor(0, L1->getHeader()); 

    // *b1 = L1->getExitBlock(); 
    // errs()<<"Drop all references\n";
    // b2->dropAllReferences(); 

    // errs()<<"removeBlock\n";    
    // b2->eraseFromParent(); 
    // LI.removeBlock(b2);
    // errs()<<"Remove Block From Loop\n";  
    // L0->getParentLoop()->removeBlockFromLoop(b2); 
    // deleteBlock(b2); 
    // ^ sarebbe eraseFromParent()
    return; 
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
