#pragma once

#include "../ast/ast_nodes.h"
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

namespace aithon::codegen {

    class AsyncTransformer {
    private:
        llvm::IRBuilder<>& builder_;
        llvm::Function* spawn_fn_;
        llvm::Function* send_fn_;
        llvm::Function* receive_fn_;

    public:
        AsyncTransformer(llvm::IRBuilder<>& builder,
                         llvm::Function* spawn,
                         llvm::Function* send,
                         llvm::Function* receive);

        // Transform async function to actor-based implementation
        llvm::Function* transform_async_function(
            ast::FunctionDef* func,
            llvm::Function* llvm_func
        );

        // Transform await expression to message receive
        llvm::Value* transform_await(
            ast::Await* await_expr,
            llvm::Function* current_func
        );

    private:
        // Generate actor spawn code
        llvm::Value* generate_spawn(llvm::Function* behavior);

        // Generate message send code
        void generate_send(llvm::Value* from_actor,
                          llvm::Value* to_actor,
                          llvm::Value* message_data,
                          llvm::Value* message_size);

        // Generate message receive code
        llvm::Value* generate_receive(llvm::Value* actor);
    };

} // namespace pyvm::codegen