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
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
// L'include seguente va in LocalOpts.h
// #include <llvm/IR/Constants.h>

using namespace llvm;

bool runOnBasicBlock(BasicBlock &B) {
    
    /*    CODICE DEL PROOOOF - inutile per ora!! 
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

    */

    // ESERCIZIO 2 - un passo piu utile
    // sostituiamo tutte le operazioni di MOLTIPLICAZIONE per due** con uno SHIFT appropriato. 
    LLVMContext &context = B.getContext();
    for (Instruction &instIter : B)
    {
      //controllo che sia un'operazione
      if(auto *BinOp = dyn_cast<BinaryOperator>(&instIter)) {
        //controllo che sia una mul 
        if (BinOp->getOpcode() == Instruction::Mul) {
          //controllo che il secondo operando sia una costante. 
          //NOTA: Suppongo che Il primo operando è sempre un registro (?) V/F?
          if (auto *value_LHS = dyn_cast<ConstantInt>(BinOp->getOperand(1))) {
            APInt is_even = value_LHS->getValue();
              //infine controllo che il secondo operando sia divisibile per due 
              if(is_even.isPowerOf2()) {
                //arrivati fin qui, devo eseguire il rimpiazzo di questa istruzione.  

                //capire di quanto devo shiftare: 
                // APInt n_shl = is_even.logBase2(); 

                // Crea il ConstantInt con il valore desiderato
                ConstantInt *constantTwo = ConstantInt::get(Type::getInt32Ty(context), is_even.logBase2());

                //creo la nuova istruzione: 
                /*constantTwo è un valore ConstantInt che eredita da Value (di molte generazioni),
                    per cui è riconosciuto come Value */
                Instruction *NewInst = BinaryOperator::Create(
                  Instruction::Shl, instIter.getOperand(0), constantTwo); 

                //ora rimpiazzo e aggiorno gli usi
                NewInst->insertAfter(&instIter);
                instIter.replaceAllUsesWith(NewInst);
              }
            }
          
        }
      }
    }

    return true; 
  }

/* APPUNTI DEL PROF
    // Manipolazione delle istruzioni
    Instruction *NewInst = BinaryOperator::Create(
        Instruction::Add, Inst1st.getOperand(0), Inst1st.getOperand(0));

    NewInst->insertAfter(&Inst1st);
    // Si possono aggiornare le singole references separatamente?
    // Controlla la documentazione e prova a rispondere.
    Inst1st.replaceAllUsesWith(NewInst);
*/

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

