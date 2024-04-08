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
#include "llvm/IR/IRBuilder.h"
// L'include seguente va in LocalOpts.h
#include <llvm/IR/Constants.h>

using namespace llvm;

bool isPunto1(BasicBlock &B)
{
    //PUNTO 1
	//Per cancellare le righe di codice che contengono il "x+0=0+" || "x*1=1*x"
	    std::vector<Instruction *> InstToDelete;
	    //Parto prendendo il basic block
	    //Ne scorro tutte le istruzioni
	    for (BasicBlock::iterator I =B.begin(); I != B.end(); ++I) 
	    {
	            Instruction &Inst = &*I;
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
		}
    //Ora cancello tutte le istruzioni che devo cancellare
	    for (auto inst : InstToDelete)
	    {
	        inst->eraseFromParent();
	    }
	//FINE PUNTO 1
    return true;
}

bool isPunto2(BasicBlock &B)
{
     IRBuilder<> Builder(B.getContext());

    //Parto scorrendo tutte le istruzioni del Basic Block
    for (BasicBlock::iterator I = B.begin(); I != B.end(); ++I)
  {
      Instruction &Inst = *I;
      //Guardo se è una operazione binaria...
      if (auto *BinOp = dyn_cast<BinaryOperator>(&Inst)) 
      {            
      //PRIMA PARTE DEL SECONDO PUNTO 
            //15 * x = x * 15 -> (𝑥 << 4) – x
        if (BinOp->getOpcode() == Instruction::Mul) 
        {
          //ora dovrei controllare che (LHS?) RHV sia una costante
          if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) 
          {
            //uso la variabile di tipo APInt perchè è quello che restituisce la funzione getValue()
            APInt LHSValue = CI->getValue();

            if(LHSValue.urem(2)) {
              //NOTAA: vedi se si può mettere al di fuori dei for. 

              //La funzione utilizzata arrotonda per eccesso, per questo motivo sottraggo uno. 
              Value *ShiftValue = Builder.getInt32(LHSValue.nearestLogBase2()-1);
              outs() << "Valore del shift (dovrebbe essere 4): " << ShiftValue << "\n"; 
              //Ora calcolo il (x<<4)
              Instruction *shlInst = BinaryOperator::Create(
                Instruction::Shl, BinOp->getOperand(0), ShiftValue); 

              //Creo la nuova istruzione che è una sottrazione tra x-shiftato e la x stessa
              Instruction *subInst = BinaryOperator::Create(
                Instruction::Sub, shlInst, BinOp->getOperand(0)); 
              
              //ora rimpiazzo e aggiorno gli usi
              shlInst->insertAfter(&Inst); 
              subInst->insertAfter(shlInst);
              Inst.replaceAllUsesWith(subInst);
              //Inst.eraseFromParent();
            }
          }
        }

          
        //...del tipo div
        if (BinOp->getOpcode() == Instruction::SDiv) 
        {
          //ora dovrei controllare che il numero costante(ipotizzando si trovi a destra(Right Hand Side) nella operazione) sia un multiplo di due
          if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) 
          {
            //uso la variabile di tipo APInt perchè è quello che restituisce la funzione getValue()
            APInt RHSValue = CI->getValue();
            //Controllo se è un multiplo di due con la funzione trovata nella classe APInt
            if (RHSValue.isPowerOf2())
            {
              /**Ora che so se è una potenza di due, devo sostituire l'istruzione con quella dello shift a sinistra
                Devo prima scoprire come si chiama lo shift a destra(perchè le punte puntano verso destra >>)**/
                    Value *ShiftAmount = Builder.getInt32(RHSValue.logBase2());
                    
                    Instruction *NewInst = BinaryOperator::Create(
                      Instruction::LShr, BinOp->getOperand(0), ShiftAmount);
                    NewInst->insertAfter(&Inst);
                    Inst.replaceAllUsesWith(NewInst);
                    //Inst.eraseFromParent();
            }
          }
        }
      }
    }
  return true;
}

bool isPunto3(BasicBlock &B)
{
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
	return true;
    //FINE PUNTO 3
}

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

    isPunto1(B);
    isPunto2(B);
    isPunto3(B);

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
