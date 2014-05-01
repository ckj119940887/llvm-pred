#include "Resolver.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include "debug.h"

using namespace std;
using namespace lle;
using namespace llvm;

Instruction* NoResolve::operator()(Value* V)
{
   Instruction* I = dyn_cast<Instruction>(V);
   Assert(I,*I);
   if(!I) return NULL;
   if(isa<LoadInst>(I) || isa<StoreInst>(I))
      //return dyn_cast<Instruction>(I->getOperand(0));
      return I;
   else if(isa<CallInst>(I))
      return NULL; // not correct
   else
      Assert(0,*I);
}

Instruction* UseOnlyResolve::operator()(Value* V)
{
   Use* Target = NULL, *Keep = NULL;
   if(LoadInst* LI = dyn_cast<LoadInst>(V)){
      Keep = Target = &LI->getOperandUse(0);
   }else if(CastInst* CI = dyn_cast<CastInst>(V)){
      Keep = Target = &CI->getOperandUse(0);
   }else if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(V)) {
      Keep = Target = &GEP->getOperandUse(0); /* FIXME this is not good because
                                                GEP has many operands*/
   }else{
      Assert(0,*V);
      return NULL;
   }
   SmallVector<CallInst*, 8> CallCandidate;
   while( (Target = Target->getNext()) ){
      //seems all things who use target is after target
      auto U = Target->getUser();
      if(isa<StoreInst>(U) && U->getOperand(1) == Target->get())
         return dyn_cast<Instruction>(U);
      if(CallInst* CI = dyn_cast<CallInst>(U)) 
         CallCandidate.push_back(CI);
   }
   if(CallCandidate.size() == 1)
      return CallCandidate[0];
   if(ConstantExpr* CE = dyn_cast<ConstantExpr>(Keep->get())){
      Instruction* TI = CE->getAsInstruction();
      return (*this)(TI);
   }

   return NULL;
}


list<Value*> ResolverBase::direct_resolve( Value* V, unordered_set<Value*>& resolved, function<void(Value*)> lambda)
{
   std::list<llvm::Value*> unresolved;
   if(resolved.find(V) != resolved.end())
      return unresolved;
   if(llvm::isa<Argument>(V))
      unresolved.push_back(V);
   if(isa<Constant>(V)){
      resolved.insert(V);
      lambda(V);
   }
   if(Instruction* I = dyn_cast<Instruction>(V)){
      if(isa<LoadInst>(I) || isa<StoreInst>(I) ){
         unresolved.push_back(I);
      }else{
         resolved.insert(I);
         lambda(I);
         for(unsigned int i=0;i<I->getNumOperands();i++){
            Value* R = I->getOperand(i);
            auto rhs = direct_resolve(R, resolved, lambda);
            unresolved.insert(unresolved.end(), rhs.begin(), rhs.end());
         }
      }
   }
   return unresolved;
}


ResolveResult ResolverBase::resolve(llvm::Value* V, std::function<void(Value*)> lambda)
{
   using namespace llvm;
   //bool changed = false;
   std::unordered_set<Value*> resolved;
   std::list<Value*> unsolved;
   std::unordered_map<Value*, Instruction*> partial;

   unsolved.push_back(NULL); // always make sure Ite wouldn't point to end();

   auto Ite = unsolved.begin();
   Value* next = V; // wait to next resolved;

   while(next || *Ite != NULL){
      if(next){
         list<Value*> mid = direct_resolve(next, resolved, lambda);
         unsolved.insert(--unsolved.end(),mid.begin(),mid.end()); 
            // insert before NULL, makes NULL always be tail
         next = NULL;
         if(*Ite == NULL) advance(Ite, -mid.size()); // *Ite == NULL means *Ite points 
            //tail, and we insert before it, we should put forware what we
            //inserted 
         continue;
      }

      Instruction* I = dyn_cast<Instruction>(*Ite);
      if(!I){
         ++Ite;
         continue;
      }

      Instruction* res = deep_resolve(I);
      partial.insert(make_pair(I,res));
      if(!res){
         ++Ite;
         continue;
      }

      resolved.insert(*Ite); // original is resolved;
      lambda(*Ite);
      Ite = unsolved.erase(Ite);
      if(isa<StoreInst>(res) || isa<LoadInst>(res)){
         resolved.insert(res);
         lambda(res);
         next = res->getOperand(0);
      }else if(isa<CallInst>(res)){
         // FIXME : wait implement
         // special call inst process
         // in this case, a def is depend on call's param
         // means we need go into called function and mark
         next = res;
      }else
         next = res;
   }

   return make_tuple(resolved, unsolved, partial);
}

