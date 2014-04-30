#ifndef SLASHSHRINK_H_H
#define SLASHSHRINK_H_H
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Pass.h>

#include "Resolver.h"

namespace lle {
   struct MarkPreserve;
   class SlashShrink;
};

struct lle::MarkPreserve
{
   static llvm::StringRef MarkNode;
   static bool enabled();
   //mark a single instuction
   static void mark(llvm::Instruction* Inst);
   //mark recusively instructions
   static void mark_all(llvm::Value* V, ResolverBase& R);
   //a helper to use mark_all with a resolver
   template<typename T>
   static void mark_all(llvm::Value* V);
   static bool is_marked(llvm::Instruction* Inst)
   {
      llvm::MDNode* MD = Inst->getMetadata(MarkNode);
      if(MD) return MD->getOperand(0) != NULL;
      return false;
   }
};

template<typename T>
void lle::MarkPreserve::mark_all(llvm::Value* V)
{
   lle::Resolver<T> R;
   mark_all(V, R);
}

class lle::SlashShrink: public llvm::FunctionPass
{
   public:
      static char ID;
      SlashShrink():FunctionPass(ID) {}

      bool runOnFunction(llvm::Function& F);
};
#endif
