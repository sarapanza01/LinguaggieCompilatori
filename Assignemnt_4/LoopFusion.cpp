#include "llvm/Transforms/Utils/LoopFusion.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

//Punto 1
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

//Il secondo punto mi chiede di vedere se i due loop hanno lo stesso numero di iterazioni
//Ho scoperto una funzione che conta proprio il trip count del loop
int getLoopIterations(ScalarEvolution &SE, Loop *L)
{
        int NumberOfIterations = SE.getSmallConstantTripCount(L);
        return NumberOfIterations + 1;
}

//Vorrei iniziare a lavorare sul punto 3, creando una funzione apposita che verifichi se due loop sono 
//Control Flow Equivalent. Per scrivere l'algoritmo che fa questa operazione, mi baserò su 
//quanto scritto nelle slide del prof
bool isControlFlowEquivalent(DominatorTree &DT, PostDominatorTree &PDT, Loop *L0, Loop *L1)
{
        //Due loop L0 e L1 si dicono control flow equivalent
        //se quando uno esegue è garantito che esegua anche l’altro.
        //Se L0 domina L1 e L1 postdomina L0 allora i due loop sono control flow equivalenti

        //Posso dire che L0 domina L1 se l'ultimo blocco di L0 domina il primo blocco di L1(ovvero il suo preheader)
        if (DT.dominates(L0->getHeader(), L1->getHeader()) && PDT.dominates(L1->getHeader(), L0->getHeader())) 
        {
            return true;
        } 
        else 
        {
            return false;
        }
}

/*                      PUNTO 4                 */
//mi serve controllare che non accedano la stessa zona di mem in interazioni diverse. 
//in qual caso, non sono validi per ottimizzazione...
//se sono validi per l'ottimizzazione, bisognerà implementare lo scambio di istruzioni. 
bool haveValidDep(DependenceInfo &DI, Loop* L0, Loop* L1)
{
        //devo estrarre ogni istruzione dei loop dentro ai blocchi che li compongono
        for (BasicBlock *BB0 : L0->getBlocks()) 
        {
                for (BasicBlock *BB1 : L1->getBlocks()) 
                {
                        for (Instruction &I0 : *BB0) 
                        {
                                for (Instruction &I1 : *BB1) 
                                {
                                        auto dep = DI.depends(&I0, &I1, true);
                                        if (dep) //hanno dipendenza
                                        {
                                                outs() << "C'è dipendenza tra le istruzioni: " << I0 <<
                                                        " e " << I1 << "\n"; 
                                                if(dep->isAnti()) //ma la dep non è valida
                                                {
                                                        errs() << "C'è dipendenza negativa tra le istruzioni (quindi non valida)"; 
                                                        return false; 
                                                }
                                                return true; 
                                        }


                                }
                        }
                }
        }
        //non hanno dipendenza
        return false; 
}

PreservedAnalyses LoopFusion::run(Function &F,
FunctionAnalysisManager &AM) {
        //Per il punto 1
        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
        //Per il punto 2
        ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
        //Per il punto 3
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
        //Per il punto 4
        DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
        
        for (auto &L0 : LI) {
                //per controllare se il punto 2 funziona
                outs() << "Il loop fa " << getLoopIterations(SE,L0) << " iterazioni\n";
               // Example: Iterate over all pairs of loops and check adjacency
               for (auto &L1 : LI) {
                   if (L0 != L1) { // Ensure we're not comparing the same loop
                   //TODO (?) : E se stessimo comparando due volte la stessa coppia di loop? 
                   outs() << "Confronto tra due loop: " << L0 << " e " << L1 <<"\n"; 
                       if (areLoopsAdjacent(L0, L1)) {
                           errs() << "Loops are adjacent\n";
                       }
                           //Ora applico il secondo punto
                        if (getLoopIterations(SE,L0) == getLoopIterations(SE,L1))
                        {
                                if (isControlFlowEquivalent(DT,PDT,L0,L1))
                                {
                                        outs() << "I loop sono entrambi eseguiti\n";

                                }
                                else
                                {
                                        outs() << "I loop non sono Control Flow Equivalent\n";
                                }
                        }
                        if(haveValidDep(DI, L0, L1))
                                outs() << "Hanno dipendenze i loop\t" << L0 
                                        << " e " << L1  << "\n"; 
                   }
               }
           }
           return PreservedAnalyses::all();
}
