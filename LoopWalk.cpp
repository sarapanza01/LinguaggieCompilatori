/**#include "llvm/Transforms/Utils/LoopWalk.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"

using namespace llvm;

PreservedAnalyses LoopWalk::run(Loop &L,
				LoopAnalysisManager &LAM,
				LoopStandardAnalysisResults &LAR,
				LPMUpdater &LU)
{
	//LoopInfo &LI = L.getLoopInfo();
	if (!L.isLoopSimplifyForm())
	{
		outs() << "Il loop non è in forma normale \n";
		return PreservedAnalyses::all();
	}
	outs() << "Il loop è in forma normale\n";
	BasicBlock *head = L.getHeader();
	Function *F = head->getParent();
	outs() << "Il CFG della funzione\n";
	for (auto Iter = F->begin(); Iter != F->end(); ++Iter)
	{
		BasicBlock &BB = *Iter;
		outs() << BB <<"\n";
	}
	outs() <<"*********************************************\n";
	outs() <<"Il LOOP:\n";	
	for (auto BI = L.block_begin(); BI != L.block_end(); ++BI)
	{
		BasicBlock *BB = *BI;
		outs() << *BB << "\n";
	}
	//Per ogni operando lo trasformo in un'istruzione, in questo modo
	//posso trovarne le reaching definitions
	//a questo putno procedo con la definizione di tutto
	//
	//Parto scorrendo tutti i blocchi,e segnandomi il preheader di un blocco
	BasicBlock *PreHeader = L.getLoopPreheader();
	//Se non c'è un preheader si ferma tutto
	if (!PreHeader)
	{
		PreservedAnalyses::all();
	}
	//Creo un vettore che contiene le istruzioni che vanno spostate
	std::vector<Instruction *> InstToMove;
	//Scorro tutto il blocco
	for(auto BI=L.block_begin(); BI != L.block_end(); ++BI)
	{
		BasicBlock *BB = *BI;
		if (BB == PreHeader)
		{
			continue;
		}
		for(auto &Inst : *BB)
		{
			for(auto *Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter)
			{
				Value *Operand = *Iter;
				if(L.isLoopInvariant(Operand))
				{
					outs() << "Il valore " << *Operand << " è loop invariant\n";
				}
			}
		}
	}
	return PreservedAnalyses::all();
}**/
#include "llvm/Transforms/Utils/LoopWalk.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/GenericLoopInfo.h"

using namespace llvm;

//Funzione che mi verifica se una certa istruzione di un certo loop è loop invariant
bool myIsLoopInvariant(Instruction &Inst, Loop &L)
{
	for (auto *Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter)
	{
		Value *Operand = Iter->get();
		//Controllo che non sia un PHINode
		if(isa<PHINode>(Inst) || isa<BranchInst>(Inst))
		{
			return false;
		}
		//Controllo che non sia una costante
		if(Instruction *arg = dyn_cast<Instruction>(Operand))
		{
			//Controllo che la variabile sia dichiarata nel loop
			if(L.contains(arg))
			{
				//In questo modo controllo iterativamente se
				//tutti gli operandi di una funzione sono 
				//o meno loop invariant
				if(!myIsLoopInvariant(*arg,L))
					return false;
			}
		}
	}
	return true;
}

//Funzione che controlla se una certa istruzione domina le uscite del //loop
bool dominatesExits(Instruction *Inst, DominatorTree &DT, Loop &L)
{	
	//vettore in cui mi segno le uscite
	std::vector<BasicBlock*> exits;
	for (auto BI: L.getBlocks())
	{
		if (BI != L.getHeader() && L.isLoopExiting(BI))
		{
			exits.push_back(BI);
		}
	}
	
	//Scorro le uscite e verifico se l’istruzione appartiene
	//ad una di queste
	for(auto *exit: exits)
	{
		if (!DT.dominates(Inst->getParent(), exit))
		{
			return false;
		}
	}
	return true;
}

//Funzione che verifica se un’istruzione domina I blocchi
//che fanno riferimento a lei in altri blocchi
bool dominatesUseBlocks(Instruction *Inst, DominatorTree &DT)
{
	for (auto Iter = Inst->user_begin(); Iter != Inst->user_end(); ++Iter)
	{
		if(!DT.dominates(Inst, dyn_cast<Instruction>(*Iter)))
		{
			return false;
		}
	}
	return true;
}

bool runOnLoop(Loop &L, LoopAnalysisManager &LAM,LoopStandardAnalysisResults &LAR,LPMUpdater &LU)
{
	//Se il loop non è nella forma semplificata, è inutile continuare
	if (!L.isLoopSimplifyForm())
	{
		outs() << "Il loop non è in forma normalizzata\n";
		return false;
	}
	//Creo il dominance tree
	//DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
	DominatorTree &DT = LAR.DT;
	//Creo un vettore per le istruzioni che devo spostare
	std::vector<Instruction*> InstToMove;
	//Creo un vettore per le istruzioni Loop Invariant
	std::vector<Instruction*> InstLI;
	
	//Itero sui BasicBlock del loop
	int i=1; //Per contare I blocchi
	for (auto BI = L.block_begin(); BI != L.block_end(); ++BI)
	{
		BasicBlock *BB = *BI;
		outs() << "Blocco" << i << "\n";
		for (auto &Inst : *BB)
		{
			if(myIsLoopInvariant(Inst, L))
			{
				outs() << Inst.getOpcodeName() << "é loop invariant\n";
				InstLI.push_back(&Inst);
			}
		}
}
	//Ora scrollo le istruzioni che si possono spostare e
	//controllo che siano in blocchi che dominano tutte le 
	//uscite del loop e che dominano tutti I blocchi del loop
	//che usano la variabile
	for (auto *inst : InstLI)
	{
		if (dominatesExits(inst, DT,L) && dominatesUseBlocks(inst,DT))
		{
			InstToMove.push_back(inst);
		}
	}
	//Ora devo spostare queste istruzioni nel preheader
	BasicBlock *preHeader = L.getLoopPreheader();
	for(auto inst : InstToMove)
	{
		outs() << *inst << "è da spostare\n";
		inst->moveBefore(&preHeader->back());
	}

return true;
}

PreservedAnalyses LoopWalk::run (Loop &L, 
				LoopAnalysisManager &LAM,
				LoopStandardAnalysisResults &LAR,
				LPMUpdater &LU)
{
	if(runOnLoop(L, LAM, LAR, LU))
	{
		PreservedAnalyses::all();
	}
	PreservedAnalyses::none();
}
