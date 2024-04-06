//===-- LocalOpts.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
// L'include seguente va in LocalOpts.h
#include <llvm/IR/Constants.h>

using namespace llvm;

bool runOnBasicBlock(BasicBlock &B) {
    
    // Preleviamo le prime due istruzioni del BB
    Instruction &Inst1st = *B.begin(), &Inst2nd = *(++B.begin());

    // L'indirizzo della prima istruzione deve essere uguale a quello del 
    // primo operando della seconda istruzione (per costruzione dell'esempio)
    assert(&Inst1st == Inst2nd.getOperand(0));

    // Stampa la prima istruzione
    outs() << "PRIMA ISTRUZIONE: " << Inst1st << "\n";
    // Stampa la prima istruzione come operando
    outs() << "COME OPERANDO: ";
    Inst1st.printAsOperand(outs(), false);
    outs() << "\n";

    // User-->Use-->Value
    outs() << "I MIEI OPERANDI SONO:\n";
    for (auto *Iter = Inst1st.op_begin(); Iter != Inst1st.op_end(); ++Iter) {
      Value *Operand = *Iter;

      if (Argument *Arg = dyn_cast<Argument>(Operand)) {
        outs() << "\t" << *Arg << ": SONO L'ARGOMENTO N. " << Arg->getArgNo() 
	       <<" DELLA FUNZIONE " << Arg->getParent()->getName()
               << "\n";
      }
      if (ConstantInt *C = dyn_cast<ConstantInt>(Operand)) {
        outs() << "\t" << *C << ": SONO UNA COSTANTE INTERA DI VALORE " << C->getValue()
               << "\n";
      }
    }

    outs() << "LA LISTA DEI MIEI USERS:\n";
    for (auto Iter = Inst1st.user_begin(); Iter != Inst1st.user_end(); ++Iter) {
      outs() << "\t" << *(dyn_cast<Instruction>(*Iter)) << "\n";
    }

    outs() << "E DEI MIEI USI (CHE E' LA STESSA):\n";
    for (auto Iter = Inst1st.use_begin(); Iter != Inst1st.use_end(); ++Iter) {
      outs() << "\t" << *(dyn_cast<Instruction>(Iter->getUser())) << "\n";
    }

    // Manipolazione delle istruzioni
    Instruction *NewInst = BinaryOperator::Create(
        Instruction::Add, Inst1st.getOperand(0), Inst1st.getOperand(0));

    NewInst->insertAfter(&Inst1st);
    // Si possono aggiornare le singole references separatamente?
    // Controlla la documentazione e prova a rispondere.
    Inst1st.replaceAllUsesWith(NewInst);

	//PUNTO 1
	//Per cancellare le righe di codice che contengono il "x+0=0+" || "x*1=1*x"
	    std::vector<Instruction *> InstToDelete;
	    //Parto prendendo il basic block
	    //Ne scorro tutte le istruzioni
	        for (BasicBlock::iterator I =B.begin(); I != B.end(); ++I) 
	        {
	            Instruction &Inst = *I;
	            //Ora controllo che la mia sia una operazione
	            if (auto *BinOp = dyn_cast<BinaryOperator>(&Inst)) 
	            {
	                //controllo che l'operando sia un add
	                if (BinOp->getOpcode() == Instruction::Add) 
	                {
	                    //Prima controllo se è nella forma "0+x"
	                    if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(0))) 
	                    {
	                        if (CI->isZero()) 
	                        {
	                            //Mi salvo il valore della x in una variabile Value
	                            Value *X = BinOp->getOperand(1);
	                            //Rimpiazzo tutte le occorrenze con la x stessa
	                            BinOp->replaceAllUsesWith(X);
	                            //Aggiungo l'istruzione a quelle da cancellare
	                            InstToDelete.push_back(Inst);
	                        }
	                    } 
	                    //Ora controllo se è nella forma "x+0"
	                    else if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) 
	                    {
	                        if (CI->isZero()) 
	                        {
	                            Value *X = BinOp->getOperand(0);
	                            BinOp->replaceAllUsesWith(X);
	                            InstToDelete.push_back(Inst);
	                        }
	                    }
	                //SECONDA PARTE
	                //controllo che l'operando sia una mul
	                if (BinOp->getOpcode() == Instruction::Mul) 
	                {
	                    //Prima controllo se è nella forma "1*x"
	                    if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(0))) 
	                    {
	                        if (CI->isOne()) 
	                        {
	                            //Mi salvo il valore della x in una variabile Value
	                            Value *X = BinOp->getOperand(1);
	                            //Rimpiazzo tutte le occorrenze con la x stessa
	                            BinOp->replaceAllUsesWith(X);
	                            InstToDelete.push_back(Inst);
	                        }
	                    } 
	                    //Ora controllo se è nella forma "x*1"
	                    else if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) 
	                    {
	                        if (CI->isOne()) 
	                        {
	                            Value *X = BinOp->getOperand(0);
	                            BinOp->replaceAllUsesWith(X);
	                            InstToDelete.push_back(Inst);
	                        }
	                    }
	                }
	            }
	        }
	    //Ora cancello tutte le istruzioni che devo cancellare
	    for (auto inst : InstToDelete)
	    {
	        inst->eraseFromParent();
	    }
	//FINE PUNTO 1

			//INIZIO PUNTO 3
			 // prelevo l'istruzione una ad una
        Instruction &Inst1st = Inst;
        // controllo se l'istruzione è un'addizione
        if (Inst.getOpcode() == Instruction::Add) {
                // controllo se il secondo operando è una costante
                Value *Const1 = Inst.getOperand(1);
                if (isa<ConstantInt>(Const1)) {
                        // Itero su tutti gli user di Inst1st
                        for (auto UserIter = Inst1st.user_begin(); UserIter != Inst1st.user_end(); ++UserIter) {
                                if(Instruction *UserInst = dyn_cast<Instruction>(*UserIter)){
                                        // Controllo che esista un'istruzione SUB (c=a-1)
                                        if(UserInst->getOpcode() == Instruction::Sub){
                                                // Controllo se l'operando Inst1st è usato come primo operando
                                                if (UserInst->getOperand(0) == &Inst1st) {
                                                        // Controllo se il secondo operando dell'istruzione utilizzatrice è una costante uguale a Const1
                                                        if (ConstantInt *Const2 = dyn_cast<ConstantInt>(UserInst->getOperand(1))){
                                                                if(Const1 == Const2){
                                                                // Sostituisco l'utilizzo dell'istruzione utilizzatrice con Inst1st
                                                                UserInst->replaceAllUsesWith(Inst1st.getOperand(0));
                                                                }
                                                        }
                                                }
                                        }
                                }

                        }
                }
        }
    }
    //FINE PUNTO 3
    return true;
  }


bool runOnFunction(Function &F) {
  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }

  return Transformed;
}


PreservedAnalyses LocalOpts::run(Module &M,
                                      ModuleAnalysisManager &AM) {
  for (auto Fiter = M.begin(); Fiter != M.end(); ++Fiter)
    if (runOnFunction(*Fiter))
      return PreservedAnalyses::none();
  
  return PreservedAnalyses::all();
}

