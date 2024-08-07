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

using namespace llvm;

bool runOnBasicBlock(BasicBlock &B) {
    LLVMContext &context = B.getContext();
 
	  //Per cancellare le righe di codice che contengono il "x+0=0+" || "x*1=1*x"
	  std::vector<Instruction *> InstToDelete;
    IRBuilder<> Builder(context);

	    //Parto prendendo il basic block
	    //Ne scorro tutte le istruzioni
	        for (Instruction &instIter : B) 
	        {
	          //Ora controllo che la mia sia una operazione
	          if (auto *BinOp = dyn_cast<BinaryOperator>(&instIter)) 
	          {
              //      LEGENDA 
                  /*      PUNTO [0]   -   ESERCIZIO 2 - un passo piu utile
                        sostituire tutte le operazioni di moltiplicazione per due con una shift
                  */

                  /*      PUNTO [1]   -   ALGEBRAIC IDENTITY: 
                        (1)   x+0 = 0+x = x   - [1] PRIMA PARTE
                        (2)   x*1 = 1*x = x   - [1] SECONDA PARTE
                  */

                  /*       PUNTO [2]   -   STRENGTH REDUCTION: 
                        (1)   15*x = x*15 -> (x<<4) - x   - [2] PRIMA PARTE
                        (2)   y = x / 8 -> y = x >>3      - [2] SECONDA PARTE
                  */

                  /*      PUNTO [3] - MULTI-INSTRUCTION OPTIMIZATION
                              a = b + 1, c = a - 1 -> a = b + 1, c = b
                  */


                  // [1] PRIMA PARTE
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
                            InstToDelete.push_back(&instIter);
                            outs() << "IsZero transform\n"; 
                        }	        
                    } 
                    //Ora controllo se è nella forma "x+0"
                    else if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) 
                    {
                        if (CI->isZero()) 
                        {
                            Value *X = BinOp->getOperand(0);
                            BinOp->replaceAllUsesWith(X);
                            InstToDelete.push_back(&instIter);
                            outs() << "IsZero transform\n"; 
                        }
                    }


                      //TODO PUNTO 3 X FATI: CONTROLLA QUESTE MODIFICHE SULLA TUA PARTE
                      // [3]
                      if (Value *Const1 = BinOp->getOperand(1)) 
                      {
                        // controllo se il secondo operando è una costante
                        if (isa<ConstantInt>(Const1)) 
                        {
                          // Itero su tutti gli user di instIter
                          for (auto UserIter = instIter.user_begin(); UserIter != instIter.user_end(); ++UserIter)
                          {
                            if(Instruction *UserInst = dyn_cast<Instruction>(*UserIter))
                            {
                              // Controllo che esista un'istruzione SUB (c=a-1)
                              if(UserInst->getOpcode() == Instruction::Sub)
                              {
                                // Controllo se l'operando instIter è usato come primo operando
                                if (UserInst->getOperand(0) == &instIter) 
                                {
                                  // Controllo se il secondo operando dell'istruzione utilizzatrice è una costante uguale a Const1
                                  if (ConstantInt *Const2 = dyn_cast<ConstantInt>(UserInst->getOperand(1)))
                                  {
                                    if(Const1 == Const2)
                                    {
                                    // Sostituisco l'utilizzo dell'istruzione utilizzatrice con instIter
                                    UserInst->replaceAllUsesWith(instIter.getOperand(0));
                                    outs() << "Multi instruction optimization\n"; 
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
	                
	                  //controllo che l'operando sia una mul
	                  if (BinOp->getOpcode() == Instruction::Mul) 
	                  {

                    //[1] SECONDA PARTE
	                    //Prima controllo se è nella forma "1*x"
	                    if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(0))) 
	                    {
	                        if (CI->isOne()) 
	                        {
	                            //Mi salvo il valore della x in una variabile Value
	                            Value *X = BinOp->getOperand(1);
	                            //Rimpiazzo tutte le occorrenze con la x stessa
	                            BinOp->replaceAllUsesWith(X);
	                            InstToDelete.push_back(&instIter);
                              outs() << "IsOne 1*x optimization transformation\n"; 
	                        }
	                    } 
	                    //Ora controllo se è nella forma "x*1"
	                    else if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) 
	                    {
	                        if (CI->isOne()) 
	                        {
	                            Value *X = BinOp->getOperand(0);
	                            BinOp->replaceAllUsesWith(X);
	                            InstToDelete.push_back(&instIter);
                              outs() << "IsOne 1*x optimization transformation\n"; 
	                        }


                          //[0] PUNTO
                          APInt LHS_Value = CI->getValue();
                          //controllo che il secondo operando sia divisibile per due 
                          if(LHS_Value.isPowerOf2()) 
                          {
                            //capire di quanto devo shiftare: 
                            ConstantInt *constantTwo = 
                              ConstantInt::get(Type::getInt32Ty(context), LHS_Value.logBase2());

                            Instruction *NewInst = BinaryOperator::Create(
                              Instruction::Shl, instIter.getOperand(0), constantTwo); 

                            //ora rimpiazzo e aggiorno gli usi
                            NewInst->insertAfter(&instIter);
                            instIter.replaceAllUsesWith(NewInst);
                            InstToDelete.push_back(&instIter); 
                            outs() << "Mul optimization\n"; 

                          }

                          // [2] PRIMA PARTE
                          //controllo se è NON divisibile per due
                          // else if (!LHS_Value.urem(2))
                          else
                          {
                            //La funzione utilizzata arrotonda per eccesso, per questo motivo sottraggo uno. 
                            Value *ShiftValue = Builder.getInt32(LHS_Value.nearestLogBase2()-1);

                            //Ora calcolo il (x<<4)
                            Instruction *shlInst = BinaryOperator::Create(
                              Instruction::Shl, BinOp->getOperand(0), ShiftValue); 

                            //Creo la nuova istruzione che è una sottrazione tra x-shiftato e la x stessa
                            Instruction *subInst = BinaryOperator::Create(
                              Instruction::Sub, shlInst, BinOp->getOperand(0)); 
                            
                            //ora rimpiazzo e aggiorno gli usi
                            shlInst->insertAfter(&instIter); 
                            subInst->insertAfter(shlInst);
                            instIter.replaceAllUsesWith(subInst);
                            InstToDelete.push_back(&instIter); 
                            outs() << "mul odd number optimization\n"; 
                          }

	                    } 

                    }
	                }

                  // [2] SECONDA PARTE
                  //...del tipo div
                  if (BinOp->getOpcode() == Instruction::SDiv) 
                  {
                    /*ora dovrei controllare che il numero costante
                      (ipotizzando si trovi a destra(Right Hand Side) nella operazione) 
                      sia un multiplo di due
                      */
                    if (ConstantInt *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) 
                    {
                      APInt RHSValue = CI->getValue();
                      if (RHSValue.isPowerOf2())
                      {
                        //sostituire l'istruzione con quella dello shift a sinistra
                        Value *ShiftAmount = Builder.getInt32(RHSValue.logBase2());
                        
                        Instruction *NewInst = BinaryOperator::Create(
                          Instruction::LShr, BinOp->getOperand(0), ShiftAmount);
                        NewInst->insertAfter(&instIter);
                        instIter.replaceAllUsesWith(NewInst);
                        InstToDelete.push_back(&instIter); 
                        outs() << "div optimization\n"; 
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
