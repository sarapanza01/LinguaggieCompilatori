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

    for (auto &Inst : B) {
    // prelevo l'istruzione corrente
    Instruction &Inst1st = Inst;

    // controllo se l'istruzione è un'addizione
    if (Inst.getOpcode() == Instruction::Add) {
        // controllo se il secondo operando è una costante
        Value *Const1 = Inst.getOperand(1);
        if (isa<ConstantInt>(Const1)) {
            // Itero su tutti gli user di Inst1st
            for (auto UserIter = Inst1st.user_begin(); UserIter != Inst1st.user_end(); ++UserIter) {
                if(Instruction *UserInst = dyn_cast<Instruction>(*UserIter)){
                                if(UserInst->getOpcode() == Instruction::Sub){
                                        // Controllo se l'operando Inst1st è usato come primo operando

                                        if (UserInst->getOperand(0) == &Inst1st) {
                                                // Controllo se il secondo operando dell'istruzione utilizzatrice è una costante uguale a Const1
                                                if (ConstantInt *Const2 = dyn_cast<ConstantInt>(UserInst->getOperand(1))) {
                                                    if (Const1 == Const2) {
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
