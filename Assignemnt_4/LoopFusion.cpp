#include "llvm/Transforms/Utils/LoopFusion.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"

using namespace llvm;
//Vorrei iniziare a lavorare sul punto 3, creando una funzione apposita che verifichi se due loop sono 
//Control Flow Equivalent. Per scrivere l'algoritmo che fa questa operazione, mi baserò su 
//quanto scritto nelle slide del prof
bool isControlFlowEquivalent(DominatorTree &DT, PostDominatorTree &PDT, Loop &L0, Loop &L1)
{
        //Due loop L0 e L1 si dicono control flow equivalent
        //se quando uno esegue è garantito che esegua anche l’altro.
        //Se L0 domina L1 e L1 postdomina L0 allora i due loop sono control flow equivalenti

        //Posso dire che L0 domina L1 se l'ultimo blocco di L0 domina il primo blocco di L1(ovvero il suo preheader)
        if (DT.dominates(L1->getHeader(), L2->getHeader()) && PDT.dominates(L2->getHeader(), L1->getHeader())) 
        {
            return true;
        } 
        else 
        {
            return false;
        }
}

PreservedAnalyses LoopFusion::run(Function &F,
FunctionAnalysisManager &AM) {
        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);

        for (auto *L0 : LI)
        {
                for (auto *L1 : LI)
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
        }
}
