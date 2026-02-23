#pragma once

#include "../ast/ast_nodes.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <map>
#include <string>

namespace aithon::codegen {

/**
 * Automatic Async/Await → Actor Transformation
 * 
 * Python Code (unchanged):
 *   async def do_something():
 *       value = await get_value()
 *       return value
 * 
 * Compiler transforms to:
 *   1. do_something() spawns a supervisor actor
 *   2. get_value() call spawns a child actor
 *   3. await becomes message receive from child
 *   4. return becomes message send to parent
 * 
 * NO CHANGES to Python code required!
 */
class AsyncToActorTransformer {
public:
    struct ActorInfo {
        std::string function_name;
        llvm::Function* behavior_function;
        llvm::Function* spawn_wrapper;
        int parent_actor_id;  // -1 for root
        bool is_supervisor;
    };
    
private:
    llvm::Module* module_;
    llvm::IRBuilder<>& builder_;
    llvm::LLVMContext& context_;
    
    // Runtime function declarations
    llvm::Function* spawn_actor_fn_;
    llvm::Function* send_message_fn_;
    llvm::Function* receive_message_fn_;
    llvm::Function* get_current_actor_fn_;
    llvm::Function* gc_alloc_fn_;
    llvm::Function* gc_collect_fn_;
    
    // Track async functions and their actor equivalents
    std::map<std::string, ActorInfo> actor_registry_;
    
    // Current transformation context
    struct TransformContext {
        llvm::Function* current_function;
        int current_actor_id;
        std::vector<llvm::Value*> actor_children;
    };
    
    TransformContext* current_context_;
    
public:
    AsyncToActorTransformer(llvm::Module* module, 
                           llvm::IRBuilder<>& builder,
                           llvm::LLVMContext& context);
    
    // Main transformation entry point
    void transform_async_function(lexer::FunctionDef* func, 
                                  llvm::Function* llvm_func);
    
    // Transform await expression (creates child actor)
    llvm::Value* transform_await_expr(lexer::Await* await_expr);
    
    // Transform async function call (spawns actor)
    llvm::Value* transform_async_call(lexer::Call* call);
    
    // Generate supervisor actor pattern
    llvm::Function* generate_supervisor_actor(lexer::FunctionDef* func);
    
    // Generate child actor pattern
    llvm::Function* generate_child_actor(const std::string& func_name,
                                         lexer::Call* call_expr);
    
private:
    // Declare runtime functions
    void declare_runtime_functions();
    
    // Generate actor behavior function
    llvm::Function* create_actor_behavior(const std::string& name,
                                          const std::string& original_func);
    
    // Generate message passing code
    llvm::Value* generate_send_to_parent(llvm::Value* result);
    llvm::Value* generate_receive_from_child();
    llvm::Value* generate_spawn_child_actor(const std::string& child_func,
                                           std::vector<llvm::Value*> args);
    
    // Generate GC integration
    void generate_gc_setup(llvm::Function* actor_func);
    void generate_gc_cleanup(llvm::Function* actor_func);
    void insert_gc_safepoints();
    
    // Helper to check if function is async
    bool is_async_function(const std::string& func_name);
    
    // Generate actor spawn wrapper
    llvm::Function* generate_spawn_wrapper(const std::string& func_name,
                                          llvm::Function* behavior);
};

/**
 * Example Transformation:
 * 
 * Python:
 *   async def process_data(x):
 *       result = await compute(x * 2)
 *       final = await finalize(result)
 *       return final
 * 
 * Generated LLVM IR (simplified):
 * 
 * define void @process_data_actor(i8* %actor_ctx, i8* %args) {
 * entry:
 *   ; Setup per-actor GC
 *   call void @gc_actor_init(%actor_ctx)
 *   
 *   ; Extract argument
 *   %x = load i64, i8* %args
 *   
 *   ; Compute x * 2
 *   %doubled = mul i64 %x, 2
 *   
 *   ; await compute(doubled) → Spawn child actor
 *   %compute_actor = call i32 @spawn_child_actor(@compute_actor, %doubled)
 *   
 *   ; Wait for result (message receive)
 *   %result_msg = call i8* @receive_message_from(%compute_actor)
 *   %result = load i64, i8* %result_msg
 *   
 *   ; GC safepoint (can collect here)
 *   call void @gc_safepoint()
 *   
 *   ; await finalize(result) → Spawn another child actor
 *   %finalize_actor = call i32 @spawn_child_actor(@finalize_actor, %result)
 *   
 *   ; Wait for final result
 *   %final_msg = call i8* @receive_message_from(%finalize_actor)
 *   %final = load i64, i8* %final_msg
 *   
 *   ; Return to parent actor (message send)
 *   call void @send_to_parent(%actor_ctx, %final)
 *   
 *   ; Cleanup GC before exit
 *   call void @gc_actor_cleanup(%actor_ctx)
 *   ret void
 * }
 */

} // namespace pyvm::codegen