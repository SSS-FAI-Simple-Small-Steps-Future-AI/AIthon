#include "../../include/codegen/async_transformer.h"

namespace aithon::codegen {

    AsyncTransformer::AsyncTransformer(llvm::IRBuilder<>& builder,
                                       llvm::Function* spawn,
                                       llvm::Function* send,
                                       llvm::Function* receive)
        : builder_(builder),
          spawn_fn_(spawn),
          send_fn_(send),
          receive_fn_(receive) {}

    llvm::Function* AsyncTransformer::transform_async_function(
        ast::FunctionDef* func,
        llvm::Function* llvm_func) {
        // Transformation is handled in llvm_codegen.cpp for now
        return llvm_func;
    }

    llvm::Value* AsyncTransformer::transform_await(
        ast::Await* await_expr,
        llvm::Function* current_func) {
        // await becomes a receive operation
        return builder_.CreateCall(receive_fn_);
    }

    llvm::Value* AsyncTransformer::generate_spawn(llvm::Function* behavior) {
        // Not yet implemented
        return nullptr;
    }

    void AsyncTransformer::generate_send(llvm::Value* from_actor,
                                        llvm::Value* to_actor,
                                        llvm::Value* message_data,
                                        llvm::Value* message_size) {
        // Not yet implemented
    }

    llvm::Value* AsyncTransformer::generate_receive(llvm::Value* actor) {
        return builder_.CreateCall(receive_fn_);
    }

} // namespace pyvm::codegen