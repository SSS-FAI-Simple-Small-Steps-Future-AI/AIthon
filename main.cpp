#include <iostream>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main() {
    LLVMContext Context;
    auto ModulePtr = std::make_unique<Module>("test_module", Context);
    IRBuilder<> Builder(Context);

    // Create function prototype: int add(int a, int b)
    std::vector<Type*> Ints(2, Builder.getInt32Ty());
    FunctionType *FT = FunctionType::get(Builder.getInt32Ty(), Ints, false);
    Function *AddFunc = Function::Create(FT, Function::ExternalLinkage, "add", ModulePtr.get());

    // Set argument names
    auto Args = AddFunc->arg_begin();
    Value *Arg1 = &*Args++;
    Arg1->setName("a");
    Value *Arg2 = &*Args++;
    Arg2->setName("b");

    // Create basic block and add instructions
    BasicBlock *BB = BasicBlock::Create(Context, "entry", AddFunc);
    Builder.SetInsertPoint(BB);
    Value *Sum = Builder.CreateAdd(Arg1, Arg2, "sum_tmp");
    Builder.CreateRet(Sum);

    // Verify and print the resulting LLVM IR to console
    verifyFunction(*AddFunc);
    ModulePtr->print(outs(), nullptr);

    return 0;
}
