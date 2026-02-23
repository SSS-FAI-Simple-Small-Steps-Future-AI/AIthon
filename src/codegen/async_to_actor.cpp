#include "../../include/codegen/async_to_actor.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>

namespace aithon::codegen {

AsyncToActorTransformer::AsyncToActorTransformer(llvm::Module* module,
                                                 llvm::IRBuilder<>& builder,
                                                 llvm::LLVMContext& context)
    : module_(module),
      builder_(builder),
      context_(context),
      current_context_(nullptr) {
    
    declare_runtime_functions();
}

void AsyncToActorTransformer::declare_runtime_functions() {
    // int spawn_actor(void (*behavior)(void*, void*), void* args)
    llvm::FunctionType* spawn_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_),
        {
            llvm::PointerType::get(context_,0),  // behavior function
            llvm::PointerType::get(context_,0)   // args
        },
        false
    );
    spawn_actor_fn_ = llvm::Function::Create(
        spawn_type,
        llvm::Function::ExternalLinkage,
        "runtime_spawn_actor",
        module_
    );
    
    // void send_message(int from, int to, void* data, size_t size)
    llvm::FunctionType* send_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        {
            llvm::Type::getInt32Ty(context_),
            llvm::Type::getInt32Ty(context_),
            llvm::PointerType::get(context_,0),
            llvm::Type::getInt64Ty(context_)
        },
        false
    );
    send_message_fn_ = llvm::Function::Create(
        send_type,
        llvm::Function::ExternalLinkage,
        "runtime_send_message",
        module_
    );
    
    // void* receive_message()
    llvm::FunctionType* receive_type = llvm::FunctionType::get(
        llvm::PointerType::get(context_,0),
        false
    );
    receive_message_fn_ = llvm::Function::Create(
        receive_type,
        llvm::Function::ExternalLinkage,
        "runtime_receive_message",
        module_
    );
    
    // int get_current_actor_id()
    llvm::FunctionType* get_actor_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_),
        false
    );
    get_current_actor_fn_ = llvm::Function::Create(
        get_actor_type,
        llvm::Function::ExternalLinkage,
        "runtime_get_current_actor_id",
        module_
    );
    
    // void* gc_alloc(size_t size)
    llvm::FunctionType* gc_alloc_type = llvm::FunctionType::get(
        llvm::PointerType::get(context_,0),
        {llvm::Type::getInt64Ty(context_)},
        false
    );
    gc_alloc_fn_ = llvm::Function::Create(
        gc_alloc_type,
        llvm::Function::ExternalLinkage,
        "gc_alloc",
        module_
    );
    
    // void gc_collect()
    llvm::FunctionType* gc_collect_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        false
    );
    gc_collect_fn_ = llvm::Function::Create(
        gc_collect_type,
        llvm::Function::ExternalLinkage,
        "gc_collect",
        module_
    );
}

void AsyncToActorTransformer::transform_async_function(lexer::FunctionDef* func,
                                                       llvm::Function* llvm_func) {
    if (!func->is_async) {
        return;  // Not an async function
    }
    
    // Generate supervisor actor for this async function
    llvm::Function* supervisor = generate_supervisor_actor(func);
    
    // Create spawn wrapper that returns actor ID
    llvm::Function* wrapper = generate_spawn_wrapper(func->name, supervisor);
    
    // Register this actor
    ActorInfo info{
        .function_name = func->name,
        .behavior_function = supervisor,
        .spawn_wrapper = wrapper,
        .parent_actor_id = -1,
        .is_supervisor = true
    };
    actor_registry_[func->name] = info;
}

llvm::Function* AsyncToActorTransformer::generate_supervisor_actor(
    lexer::FunctionDef* func) {
    
    // Create actor behavior function
    std::string behavior_name = func->name + "_actor_behavior";
    
    llvm::FunctionType* behavior_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        {
            llvm::PointerType::get(context_,0),  // actor context
            llvm::PointerType::get(context_,0)   // args
        },
        false
    );
    
    llvm::Function* behavior = llvm::Function::Create(
        behavior_type,
        llvm::Function::InternalLinkage,
        behavior_name,
        module_
    );
    
    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        context_, "entry", behavior
    );
    builder_.SetInsertPoint(entry);
    
    // Setup GC for this actor
    generate_gc_setup(behavior);
    
    // Process function body
    // Each await becomes:
    //   1. Spawn child actor
    //   2. Receive message from child
    //   3. Continue execution
    
    // For now, generate basic structure
    // Real implementation would traverse func->body and transform each await
    
    // Example transformation for: result = await get_value()
    // 1. Spawn child actor for get_value()
    // llvm::Value* child_id = generate_spawn_child_actor("get_value", {});
    
    // 2. Receive result from child
    // llvm::Value* result_msg = generate_receive_from_child();
    
    // 3. Extract result from message
    // llvm::Value* result = builder_.CreateLoad(result_msg);
    
    // Cleanup GC before exit
    generate_gc_cleanup(behavior);
    
    builder_.CreateRetVoid();
    
    return behavior;
}

llvm::Function* AsyncToActorTransformer::generate_spawn_wrapper(
    const std::string& func_name,
    llvm::Function* behavior) {
    
    // Create wrapper function that spawns the actor
    std::string wrapper_name = func_name + "_spawn";
    
    llvm::FunctionType* wrapper_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_),  // returns actor ID
        {llvm::PointerType::get(context_,0)},  // args
        false
    );
    
    llvm::Function* wrapper = llvm::Function::Create(
        wrapper_type,
        llvm::Function::ExternalLinkage,
        wrapper_name,
        module_
    );
    
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        context_, "entry", wrapper
    );
    builder_.SetInsertPoint(entry);
    
    // Get args parameter
    llvm::Value* args = &*wrapper->arg_begin();
    
    // Cast behavior function to void*
    llvm::Value* behavior_ptr = builder_.CreateBitCast(
        behavior,
        llvm::PointerType::get(context_,0)
    );
    
    // Call spawn_actor
    llvm::Value* actor_id = builder_.CreateCall(
        spawn_actor_fn_,
        {behavior_ptr, args}
    );
    
    builder_.CreateRet(actor_id);
    
    return wrapper;
}

llvm::Value* AsyncToActorTransformer::transform_await_expr(lexer::Await* await_expr) {
    // Transform: result = await some_call()
    // Into:
    //   1. child_id = spawn_child_actor(some_call)
    //   2. result_msg = receive_message()
    //   3. result = extract_from_message(result_msg)
    
    // Check if the awaited expression is a function call
    if (await_expr->value->type == lexer::NodeType::CALL) {
        auto* call = static_cast<lexer::Call*>(await_expr->value.get());
        
        // Get function name
        if (call->func->type == lexer::NodeType::NAME) {
            auto* name_node = static_cast<lexer::Name*>(call->func.get());
            std::string func_name = name_node->id;
            
            // Spawn child actor for this function
            // (In real implementation, would pass arguments)
            std::vector<llvm::Value*> args;
            llvm::Value* child_id = generate_spawn_child_actor(func_name, args);
            
            // Receive result from child
            llvm::Value* result = generate_receive_from_child();
            
            // Insert GC safepoint (can collect while waiting)
            insert_gc_safepoints();
            
            return result;
        }
    }
    
    return nullptr;
}

llvm::Value* AsyncToActorTransformer::generate_spawn_child_actor(
    const std::string& child_func,
    std::vector<llvm::Value*> args) {
    
    // Look up child function's actor wrapper
    auto it = actor_registry_.find(child_func);
    if (it != actor_registry_.end()) {
        // Call the spawn wrapper
        llvm::Value* null_args = llvm::ConstantPointerNull::get(
            llvm::PointerType::get(context_,0)
        );
        
        return builder_.CreateCall(
            it->second.spawn_wrapper,
            {null_args}
        );
    }
    
    return nullptr;
}

llvm::Value* AsyncToActorTransformer::generate_receive_from_child() {
    // Call runtime_receive_message()
    return builder_.CreateCall(receive_message_fn_);
}

llvm::Value* AsyncToActorTransformer::generate_send_to_parent(llvm::Value* result) {
    // Get current actor ID
    llvm::Value* current_id = builder_.CreateCall(get_current_actor_fn_);
    
    // Parent ID is stored in actor context (simplified)
    // In real implementation, would extract from actor context
    llvm::Value* parent_id = builder_.getInt32(-1);  // Placeholder
    
    // Allocate message buffer from GC
    llvm::Value* msg_size = builder_.getInt64(sizeof(void*));
    llvm::Value* msg_buf = builder_.CreateCall(gc_alloc_fn_, {msg_size});
    
    // Store result in message
    builder_.CreateStore(result, msg_buf);
    
    // Send to parent
    builder_.CreateCall(
        send_message_fn_,
        {current_id, parent_id, msg_buf, msg_size}
    );
    
    return nullptr;
}

void AsyncToActorTransformer::generate_gc_setup(llvm::Function* actor_func) {
    // Setup per-actor GC at function entry
    // This would call actor GC initialization
    
    // For now, just add a comment
    // In real implementation:
    // call void @actor_gc_init(i8* %actor_ctx)
}

void AsyncToActorTransformer::generate_gc_cleanup(llvm::Function* actor_func) {
    // Cleanup GC before function exit
    // Do final collection to free all objects
    
    builder_.CreateCall(gc_collect_fn_);
    
    // In real implementation:
    // call void @actor_gc_destroy(i8* %actor_ctx)
}

void AsyncToActorTransformer::insert_gc_safepoints() {
    // Insert GC safepoint - point where GC can run
    // These are inserted at:
    //   - await points
    //   - loop backedges
    //   - function calls
    
    builder_.CreateCall(gc_collect_fn_);
}

bool AsyncToActorTransformer::is_async_function(const std::string& func_name) {
    return actor_registry_.find(func_name) != actor_registry_.end();
}

} // namespace pyvm::codegen