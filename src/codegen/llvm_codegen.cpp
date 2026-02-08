#include "../../include/codegen/llvm_codegen.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/TargetParser/Host.h>  // Contains getDefaultTargetTriple()
#include <iostream>

namespace aithon::codegen {

LLVMCodeGen::LLVMCodeGen(const std::string& module_name)
    : builder_(context_),
      module_(std::make_unique<llvm::Module>(module_name, context_)),
      current_function_(nullptr) {
    
    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();
    
    // Declare runtime functions
    declare_runtime_functions();
}

void LLVMCodeGen::declare_runtime_functions() {
    // int runtime_spawn_actor(void (*behavior)(void*, void*), void* args)
    llvm::FunctionType* spawn_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_),  // returns PID
        {
            llvm::PointerType::get(context_,0),  // behavior function
            llvm::PointerType::get(context_,0)   // args
        },
        false
    );
    runtime_spawn_actor_ = llvm::Function::Create(
        spawn_type,
        llvm::Function::ExternalLinkage,
        "runtime_spawn_actor",
        module_.get()
    );
    
    // bool runtime_send_message(int from_pid, int to_pid, void* data, size_t size)
    llvm::FunctionType* send_type = llvm::FunctionType::get(
        llvm::Type::getInt1Ty(context_),  // returns bool
        {
            llvm::Type::getInt32Ty(context_),  // from_pid
            llvm::Type::getInt32Ty(context_),  // to_pid
            llvm::PointerType::get(context_,0),  // data
            llvm::Type::getInt64Ty(context_)   // size
        },
        false
    );
    runtime_send_message_ = llvm::Function::Create(
        send_type,
        llvm::Function::ExternalLinkage,
        "runtime_send_message",
        module_.get()
    );
    
    // void* runtime_receive_message()
    llvm::FunctionType* receive_type = llvm::FunctionType::get(
        llvm::PointerType::get(context_,0),  // returns message*
        false
    );
    runtime_receive_message_ = llvm::Function::Create(
        receive_type,
        llvm::Function::ExternalLinkage,
        "runtime_receive_message",
        module_.get()
    );
    
    // bool runtime_should_yield()
    llvm::FunctionType* yield_type = llvm::FunctionType::get(
        llvm::Type::getInt1Ty(context_),  // returns bool
        false
    );
    runtime_should_yield_ = llvm::Function::Create(
        yield_type,
        llvm::Function::ExternalLinkage,
        "runtime_should_yield",
        module_.get()
    );
    
    // void runtime_print_int(int64_t)
    llvm::FunctionType* print_int_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        {llvm::Type::getInt64Ty(context_)},
        false
    );
    runtime_print_int_ = llvm::Function::Create(
        print_int_type,
        llvm::Function::ExternalLinkage,
        "runtime_print_int",
        module_.get()
    );
    
    // void runtime_print_float(double)
    llvm::FunctionType* print_float_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        {llvm::Type::getDoubleTy(context_)},
        false
    );
    runtime_print_float_ = llvm::Function::Create(
        print_float_type,
        llvm::Function::ExternalLinkage,
        "runtime_print_float",
        module_.get()
    );
    
    // void runtime_print_string(const char*)
    llvm::FunctionType* print_string_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        {llvm::PointerType::get(context_,0)},
        false
    );
    runtime_print_string_ = llvm::Function::Create(
        print_string_type,
        llvm::Function::ExternalLinkage,
        "runtime_print_string",
        module_.get()
    );
}

void LLVMCodeGen::codegen(ast::Module* module) {
    for (auto& stmt : module->body) {
        codegen(stmt.get());
    }
}

llvm::Value* LLVMCodeGen::codegen(ast::ASTNode* node) {
    if (!node) return nullptr;
    
    switch (node->type) {
        case ast::NodeType::FUNCTION_DEF:
        case ast::NodeType::ASYNC_FUNCTION_DEF:
            return codegen_function(static_cast<ast::FunctionDef*>(node));
        case ast::NodeType::RETURN:
            return codegen_return(static_cast<ast::Return*>(node));
        case ast::NodeType::ASSIGN:
            return codegen_assign(static_cast<ast::Assign*>(node));
        case ast::NodeType::BINOP:
            return codegen_binop(static_cast<ast::BinOp*>(node));
        case ast::NodeType::UNARYOP:
            return codegen_unaryop(static_cast<ast::UnaryOpNode*>(node));
        case ast::NodeType::COMPARE:
            return codegen_compare(static_cast<ast::Compare*>(node));
        case ast::NodeType::CALL:
            return codegen_call(static_cast<ast::Call*>(node));
        case ast::NodeType::AWAIT:
            return codegen_await(static_cast<ast::Await*>(node));
        case ast::NodeType::NAME:
            return codegen_name(static_cast<ast::Name*>(node));
        case ast::NodeType::CONSTANT:
            return codegen_constant(static_cast<ast::Constant*>(node));
        case ast::NodeType::IF:
            return codegen_if(static_cast<ast::If*>(node));
        case ast::NodeType::WHILE:
            return codegen_while(static_cast<ast::While*>(node));
        case ast::NodeType::FOR:
            return codegen_for(static_cast<ast::For*>(node));
        case ast::NodeType::EXPR: {
            auto expr = static_cast<ast::Expr*>(node);
            return codegen(expr->value.get());
        }
        default:
            std::cerr << "Unsupported node type in codegen\n";
            return nullptr;
    }
}

llvm::Function* LLVMCodeGen::codegen_function(ast::FunctionDef* func) {
    if (func->is_async) {
        return codegen_async_function(func);
    }
    
    // Create function type (all args and return are int64 for simplicity)
    std::vector<llvm::Type*> arg_types(func->args.size(), llvm::Type::getInt64Ty(context_));
    llvm::FunctionType* func_type = llvm::FunctionType::get(
        llvm::Type::getInt64Ty(context_),
        arg_types,
        false
    );
    
    llvm::Function* llvm_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        func->name,
        module_.get()
    );
    
    // Store function
    functions_[func->name] = llvm_func;
    current_function_ = llvm_func;
    
    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context_, "entry", llvm_func);
    builder_.SetInsertPoint(entry);
    
    // Create allocas for arguments
    size_t idx = 0;
    for (auto& arg : llvm_func->args()) {
        std::string arg_name = func->args[idx++];
        llvm::AllocaInst* alloca = create_entry_block_alloca(llvm_func, arg_name);
        builder_.CreateStore(&arg, alloca);
        named_values_[arg_name] = alloca;
    }
    
    // Generate body
    llvm::Value* ret_val = nullptr;
    for (auto& stmt : func->body) {
        ret_val = codegen(stmt.get());
    }
    
    // Verify function
    if (llvm::verifyFunction(*llvm_func, &llvm::errs())) {
        std::cerr << "Error in function: " << func->name << std::endl;
    }
    
    named_values_.clear();
    current_function_ = nullptr;
    
    return llvm_func;
}

llvm::Function* LLVMCodeGen::codegen_async_function(ast::FunctionDef* func) {
    // Async functions return actor PID
    std::vector<llvm::Type*> arg_types(func->args.size(), llvm::Type::getInt64Ty(context_));
    llvm::FunctionType* func_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context_),  // returns PID
        arg_types,
        false
    );
    
    llvm::Function* llvm_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        func->name,
        module_.get()
    );
    
    functions_[func->name] = llvm_func;
    
    // Create behavior function (what the actor will run)
    std::string behavior_name = func->name + "_behavior";
    llvm::FunctionType* behavior_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        {
            llvm::PointerType::get(context_,0),  // actor*
            llvm::PointerType::get(context_,0)   // args*
        },
        false
    );
    
    llvm::Function* behavior_func = llvm::Function::Create(
        behavior_type,
        llvm::Function::InternalLinkage,
        behavior_name,
        module_.get()
    );
    
    // Generate behavior body
    current_function_ = behavior_func;
    llvm::BasicBlock* behavior_entry = llvm::BasicBlock::Create(
        context_, "entry", behavior_func
    );
    builder_.SetInsertPoint(behavior_entry);
    
    // TODO: Extract args from args*
    // Generate function body
    for (auto& stmt : func->body) {
        codegen(stmt.get());
    }
    
    builder_.CreateRetVoid();
    
    // Now generate wrapper that spawns actor
    current_function_ = llvm_func;
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context_, "entry", llvm_func);
    builder_.SetInsertPoint(entry);
    
    // Call runtime_spawn_actor
    llvm::Value* behavior_ptr = builder_.CreateBitCast(
        behavior_func,
        llvm::PointerType::get(context_,0)
    );
    llvm::Value* null_args = llvm::ConstantPointerNull::get(
        llvm::PointerType::get(context_,0)
    );
    llvm::Value* pid = builder_.CreateCall(
        runtime_spawn_actor_,
        {behavior_ptr, null_args}
    );
    
    builder_.CreateRet(pid);
    
    named_values_.clear();
    current_function_ = nullptr;
    
    return llvm_func;
}

llvm::Value* LLVMCodeGen::codegen_return(ast::Return* ret) {
    if (ret->value) {
        llvm::Value* ret_val = codegen(ret->value.get());
        if (ret_val) {
            return builder_.CreateRet(ret_val);
        }
    }
    return builder_.CreateRetVoid();
}

llvm::Value* LLVMCodeGen::codegen_assign(ast::Assign* assign) {
    if (assign->targets.empty() || !assign->value) return nullptr;
    
    llvm::Value* val = codegen(assign->value.get());
    if (!val) return nullptr;
    
    // Simple assignment to first target (assuming it's a Name)
    if (assign->targets[0]->type == ast::NodeType::NAME) {
        auto name_node = static_cast<ast::Name*>(assign->targets[0].get());

        // Before (error):
        // llvm::AllocaInst* alloca = named_values_[name_node->id];
        // After (correct):
        llvm::AllocaInst* alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(named_values_[name_node->id]);
        if (!alloca) {
            alloca = create_entry_block_alloca(current_function_, name_node->id);
            named_values_[name_node->id] = alloca;
        }
        
        builder_.CreateStore(val, alloca);
    }
    
    return val;
}

llvm::Value* LLVMCodeGen::codegen_binop(ast::BinOp* binop) {
    llvm::Value* left = codegen(binop->left.get());
    llvm::Value* right = codegen(binop->right.get());
    
    if (!left || !right) return nullptr;
    
    switch (binop->op) {
        case ast::BinaryOp::ADD:
            return builder_.CreateAdd(left, right, "addtmp");
        case ast::BinaryOp::SUB:
            return builder_.CreateSub(left, right, "subtmp");
        case ast::BinaryOp::MULT:
            return builder_.CreateMul(left, right, "multmp");
        case ast::BinaryOp::DIV:
            return builder_.CreateSDiv(left, right, "divtmp");
        case ast::BinaryOp::MOD:
            return builder_.CreateSRem(left, right, "modtmp");
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::codegen_unaryop(ast::UnaryOpNode* unaryop) {
    llvm::Value* operand = codegen(unaryop->operand.get());
    if (!operand) return nullptr;
    
    switch (unaryop->op) {
        case ast::UnaryOp::USUB:
            return builder_.CreateNeg(operand, "negtmp");
        case ast::UnaryOp::NOT:
            return builder_.CreateNot(operand, "nottmp");
        default:
            return operand;
    }
}

llvm::Value* LLVMCodeGen::codegen_compare(ast::Compare* compare) {
    llvm::Value* left = codegen(compare->left.get());
    if (!left || compare->ops.empty() || compare->comparators.empty()) return nullptr;
    
    llvm::Value* right = codegen(compare->comparators[0].get());
    if (!right) return nullptr;
    
    switch (compare->ops[0]) {
        case ast::CompareOp::EQ:
            return builder_.CreateICmpEQ(left, right, "eqtmp");
        case ast::CompareOp::NOTEQ:
            return builder_.CreateICmpNE(left, right, "netmp");
        case ast::CompareOp::LT:
            return builder_.CreateICmpSLT(left, right, "lttmp");
        case ast::CompareOp::LTE:
            return builder_.CreateICmpSLE(left, right, "letmp");
        case ast::CompareOp::GT:
            return builder_.CreateICmpSGT(left, right, "gttmp");
        case ast::CompareOp::GTE:
            return builder_.CreateICmpSGE(left, right, "getmp");
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::codegen_call(ast::Call* call) {
    if (call->func->type != ast::NodeType::NAME) return nullptr;
    
    auto func_name_node = static_cast<ast::Name*>(call->func.get());
    std::string func_name = func_name_node->id;
    
    // Check for built-in print function
    if (func_name == "print" && !call->args.empty()) {
        llvm::Value* arg = codegen(call->args[0].get());
        if (arg) {
            if (arg->getType()->isIntegerTy()) {
                return builder_.CreateCall(runtime_print_int_, {arg});
            } else if (arg->getType()->isDoubleTy()) {
                return builder_.CreateCall(runtime_print_float_, {arg});
            }
        }
        return nullptr;
    }
    
    // Regular function call
    llvm::Function* callee = functions_[func_name];
    if (!callee) {
        callee = module_->getFunction(func_name);
    }
    
    if (!callee) return nullptr;
    
    std::vector<llvm::Value*> args;
    for (auto& arg : call->args) {
        llvm::Value* arg_val = codegen(arg.get());
        if (arg_val) args.push_back(arg_val);
    }
    
    return builder_.CreateCall(callee, args, "calltmp");
}

llvm::Value* LLVMCodeGen::codegen_await(ast::Await* await_expr) {
    // await becomes a message receive
    llvm::Value* msg_ptr = builder_.CreateCall(runtime_receive_message_);
    return msg_ptr;
}

llvm::Value* LLVMCodeGen::codegen_name(ast::Name* name) {
    llvm::Value* val = named_values_[name->id];
    if (!val) return nullptr;
    return builder_.CreateLoad(llvm::Type::getInt64Ty(context_), val, name->id.c_str());
}

llvm::Value* LLVMCodeGen::codegen_constant(ast::Constant* constant) {
    if (std::holds_alternative<int64_t>(constant->value)) {
        return llvm::ConstantInt::get(context_, llvm::APInt(64, std::get<int64_t>(constant->value)));
    } else if (std::holds_alternative<double>(constant->value)) {
        return llvm::ConstantFP::get(context_, llvm::APFloat(std::get<double>(constant->value)));
    } else if (std::holds_alternative<bool>(constant->value)) {
        return llvm::ConstantInt::get(context_, llvm::APInt(1, std::get<bool>(constant->value)));
    }
    return nullptr;
}

llvm::Value* LLVMCodeGen::codegen_if(ast::If* if_stmt) {
    llvm::Value* cond = codegen(if_stmt->test.get());
    if (!cond) return nullptr;
    
    // Convert to bool
    cond = builder_.CreateICmpNE(
        cond, llvm::ConstantInt::get(context_, llvm::APInt(64, 0)), "ifcond"
    );
    
    llvm::Function* func = builder_.GetInsertBlock()->getParent();
    llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(context_, "then", func);
    llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(context_, "else");
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context_, "ifcont");
    
    builder_.CreateCondBr(cond, then_bb, else_bb);
    
    // Then block
    builder_.SetInsertPoint(then_bb);
    for (auto& stmt : if_stmt->body) {
        codegen(stmt.get());
    }
    builder_.CreateBr(merge_bb);
    
    // Else block
    func->insert(func->end(), else_bb);
    builder_.SetInsertPoint(else_bb);
    for (auto& stmt : if_stmt->orelse) {
        codegen(stmt.get());
    }
    builder_.CreateBr(merge_bb);
    
    // Merge block
    func->insert(func->end(), merge_bb);
    builder_.SetInsertPoint(merge_bb);
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::codegen_while(ast::While* while_stmt) {
    llvm::Function* func = builder_.GetInsertBlock()->getParent();
    llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(context_, "whilecond", func);
    llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(context_, "whilebody");
    llvm::BasicBlock* after_bb = llvm::BasicBlock::Create(context_, "afterwhile");
    
    builder_.CreateBr(cond_bb);
    
    // Condition block
    builder_.SetInsertPoint(cond_bb);
    llvm::Value* cond = codegen(while_stmt->test.get());
    cond = builder_.CreateICmpNE(
        cond, llvm::ConstantInt::get(context_, llvm::APInt(64, 0)), "whilecond"
    );
    builder_.CreateCondBr(cond, body_bb, after_bb);
    
    // Body block
    func->insert(func->end(), body_bb);
    builder_.SetInsertPoint(body_bb);
    for (auto& stmt : while_stmt->body) {
        codegen(stmt.get());
    }
    builder_.CreateBr(cond_bb);
    
    // After block
    func->insert(func->end(), after_bb);
    builder_.SetInsertPoint(after_bb);
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::codegen_for(ast::For* for_stmt) {
    // Simplified for loop (not fully implemented)
    return nullptr;
}

llvm::Type* LLVMCodeGen::get_default_type() {
    return llvm::Type::getInt64Ty(context_);
}

llvm::Value* LLVMCodeGen::create_default_value() {
    return llvm::ConstantInt::get(context_, llvm::APInt(64, 0));
}

llvm::AllocaInst* LLVMCodeGen::create_entry_block_alloca(
    llvm::Function* func, const std::string& var_name) {
    llvm::IRBuilder<> tmp_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(llvm::Type::getInt64Ty(context_), nullptr, var_name);
}

void LLVMCodeGen::emit_llvm_ir(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    
    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << std::endl;
        return;
    }
    
    module_->print(dest, nullptr);
}

void LLVMCodeGen::emit_object_file(const std::string& filename) {
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    module_->setTargetTriple(llvm::Triple(target_triple));
    
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    
    if (!target) {
        std::cerr << error << std::endl;
        return;
    }
    
    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    auto target_machine = target->createTargetMachine(
        target_triple, cpu, features, opt, llvm::Reloc::PIC_
    );
    
    module_->setDataLayout(target_machine->createDataLayout());
    
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    
    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << std::endl;
        return;
    }
    
    llvm::legacy::PassManager pass;
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr,
                                            llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "Target machine can't emit object file" << std::endl;
        return;
    }
    
    pass.run(*module_);
    dest.flush();
}

void LLVMCodeGen::optimize() {
    // Basic optimization passes
    llvm::legacy::FunctionPassManager fpm(module_.get());
    fpm.add(llvm::createInstructionCombiningPass());
    fpm.add(llvm::createReassociatePass());
    fpm.add(llvm::createGVNPass());
    fpm.add(llvm::createCFGSimplificationPass());
    fpm.doInitialization();
    
    for (auto& func : *module_) {
        fpm.run(func);
    }
}

} // namespace pyvm::codegen