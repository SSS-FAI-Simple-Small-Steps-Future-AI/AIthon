#pragma once

#include "../ast/ast_nodes.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <memory>
#include <map>
#include <string>

namespace pyvm::codegen {

class LLVMCodeGen {
private:
    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    std::unique_ptr<llvm::Module> module_;
    
    // Symbol table
    std::map<std::string, llvm::Value*> named_values_;
    std::map<std::string, llvm::Function*> functions_;
    
    // Runtime function declarations
    llvm::Function* runtime_spawn_actor_;
    llvm::Function* runtime_send_message_;
    llvm::Function* runtime_receive_message_;
    llvm::Function* runtime_should_yield_;
    llvm::Function* runtime_print_int_;
    llvm::Function* runtime_print_float_;
    llvm::Function* runtime_print_string_;
    
    // Current function being compiled
    llvm::Function* current_function_;
    
public:
    explicit LLVMCodeGen(const std::string& module_name);
    ~LLVMCodeGen() = default;
    
    // Generate code for AST
    void codegen(ast::Module* module);
    
    // Get the generated module
    llvm::Module* get_module() { return module_.get(); }
    
    // Write to object file
    void emit_object_file(const std::string& filename);
    
    // Write to LLVM IR file
    void emit_llvm_ir(const std::string& filename);
    
    // Optimize module
    void optimize();
    
private:
    // Declare runtime functions
    void declare_runtime_functions();
    
    // Code generation for different node types
    llvm::Value* codegen(ast::ASTNode* node);
    llvm::Function* codegen_function(ast::FunctionDef* func);
    llvm::Function* codegen_async_function(ast::FunctionDef* func);
    llvm::Value* codegen_return(ast::Return* ret);
    llvm::Value* codegen_assign(ast::Assign* assign);
    llvm::Value* codegen_binop(ast::BinOp* binop);
    llvm::Value* codegen_unaryop(ast::UnaryOp* unaryop);
    llvm::Value* codegen_compare(ast::Compare* compare);
    llvm::Value* codegen_call(ast::Call* call);
    llvm::Value* codegen_await(ast::Await* await_expr);
    llvm::Value* codegen_name(ast::Name* name);
    llvm::Value* codegen_constant(ast::Constant* constant);
    llvm::Value* codegen_if(ast::If* if_stmt);
    llvm::Value* codegen_while(ast::While* while_stmt);
    llvm::Value* codegen_for(ast::For* for_stmt);
    
    // Helper functions
    llvm::Type* get_default_type();
    llvm::Value* create_default_value();
    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* func,
                                                  const std::string& var_name);
};

} // namespace pyvm::codegen