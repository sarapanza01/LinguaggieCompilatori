/** 
La mia idea per fare il passo 2 è partire dal mio passo 0 e adattarlo al testo del problema, farò prima la seconda
parte del punto 2 perchè è quella più simile al punto 0.
**/

#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"


using namespace llvm;

bool runOnBasicBlock(BasicBlock &B) 
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
