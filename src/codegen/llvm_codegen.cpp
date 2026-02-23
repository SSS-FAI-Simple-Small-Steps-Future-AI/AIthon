#include "../../include/codegen/llvm_codegen.h"
#include <iostream>

#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/DataLayout.h>

namespace aithon::codegen {

// ============================================================================
// Constructor & Destructor
// ============================================================================

LLVMCodeGen::LLVMCodeGen(utils::ErrorReporter& reporter)
    : error_reporter_(reporter), current_function_(nullptr) {

    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

LLVMCodeGen::~LLVMCodeGen() = default;

// ============================================================================
// Initialization
// ============================================================================

void LLVMCodeGen::initialize_module(const std::string& module_name) {
    context_ = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>(module_name, *context_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);

    // No need to set target triple explicitly - LLVM will use the default
    // The target triple is set automatically when creating the module

    setup_optimization_passes();
    declare_runtime_functions();
}

void LLVMCodeGen::setup_optimization_passes() {
    fpm_ = std::make_unique<llvm::legacy::FunctionPassManager>(module_.get());

    // Basic optimizations
    // fpm_->add(llvm::createInstructionCombiningPass());
    // fpm_->add(llvm::createReassociatePass());
    // fpm_->add(llvm::createGVNPass());
    // fpm_->add(llvm::createCFGSimplificationPass());

    fpm_->doInitialization();
}

void LLVMCodeGen::declare_runtime_functions() {
    llvm::Type* void_ty = llvm::Type::getVoidTy(*context_);
    llvm::Type* i64_ty  = llvm::Type::getInt64Ty(*context_);
    llvm::Type* dbl_ty  = llvm::Type::getDoubleTy(*context_);
    llvm::Type* i1_ty   = llvm::Type::getInt1Ty(*context_);
    llvm::Type* ptr_ty  = llvm::PointerType::getUnqual(*context_);

    auto declare = [&](const char* name,
                       llvm::Type* ret,
                       std::vector<llvm::Type*> params) {
        llvm::FunctionType* ft = llvm::FunctionType::get(ret, params, false);
        if (!module_->getFunction(name)) {
            llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                   name, module_.get());
        }
    };

    // Scalars
    declare("runtime_print_int",    void_ty, {i64_ty});
    declare("runtime_print_float",  void_ty, {dbl_ty});
    declare("runtime_print_bool",   void_ty, {i1_ty});
    declare("runtime_print_string", void_ty, {ptr_ty});

    // Collections
    declare("runtime_list_print",   void_ty, {ptr_ty});
    declare("runtime_dict_print",   void_ty, {ptr_ty});


    // void runtime_print_int(i64)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::Type::getInt64Ty(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_print_int", module_.get());
    }

    // void runtime_print_float(double)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::Type::getDoubleTy(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_print_float", module_.get());
    }

    // void runtime_print_string(ptr)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::PointerType::getUnqual(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_print_string", module_.get());
    }

    // void runtime_print_bool(i1)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::Type::getInt1Ty(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_print_bool", module_.get());
    }

    // --- List Functions ---

    // void* runtime_list_create()
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context_),
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_list_create", module_.get());
    }

    // void runtime_list_append_string(void* list, const char* str)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::PointerType::getUnqual(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_list_append_string", module_.get());
    }

    // void runtime_list_append_int(void* list, i64 val)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::Type::getInt64Ty(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_list_append_int", module_.get());
    }

    // const char* runtime_list_get_string(void* list, i64 index)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::Type::getInt64Ty(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_list_get_string", module_.get());
    }

    // i64 runtime_list_get_int(void* list, i64 index)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::Type::getInt64Ty(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_list_get_int", module_.get());
    }

    // --- Dictionary Functions ---

    // void* runtime_dict_create()
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context_),
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_dict_create", module_.get());
    }

    // void runtime_dict_set_string(void* dict, const char* key, const char* val)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::PointerType::getUnqual(*context_),
             llvm::PointerType::getUnqual(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_dict_set_string", module_.get());
    }

    // void runtime_dict_set_int(void* dict, const char* key, i64 val)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::PointerType::getUnqual(*context_),
             llvm::Type::getInt64Ty(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_dict_set_int", module_.get());
    }

    // const char* runtime_dict_get_string(void* dict, const char* key)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::PointerType::getUnqual(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_dict_get_string", module_.get());
    }

    // i64 runtime_dict_get_int(void* dict, const char* key)
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            {llvm::PointerType::getUnqual(*context_),
             llvm::PointerType::getUnqual(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_dict_get_int", module_.get());
    }
    {
        // Print collections
        llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            {llvm::PointerType::getUnqual(*context_)},
            false
        );
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_list_print", module_.get());
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                              "runtime_dict_print", module_.get());
    }
}

llvm::AllocaInst* LLVMCodeGen::create_entry_block_alloca(llvm::Function* func,
                                                         const std::string& var_name,
                                                         llvm::Type* type) {
    if (!type) {
        type = llvm::Type::getInt64Ty(*context_);
    }

    llvm::IRBuilder<> tmp_builder(&func->getEntryBlock(),
                                   func->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, nullptr, var_name);
}

// ============================================================================
// Main Code Generation
// ============================================================================

bool LLVMCodeGen::generate(parser::ast::Module* module, const std::string& module_name) {
    // std::cerr << ">>> [1] Starting\n";
    if (!module) return false;

    // std::cerr << ">>> [2] Initializing\n";
    initialize_module(module_name);


    // for (auto& stmt : module->statements) {
    //     codegen_stmt(stmt.get());
    // }

    // std::cerr << ">>> [3] Generating code\n";
    // Generate code for all statements
    for (size_t i = 0; i < module->statements.size(); i++) {
        std::cerr << ">>> [3." << (i+1) << "] Statement\n";
        codegen_stmt(module->statements[i].get());
    }

    // std::cerr << ">>> [4] Main wrapper\n";
    // Generate main() wrapper that calls python_main()
    generate_main_wrapper();

    // std::cerr << ">>> [5] Verifying\n";
    // Verify module
    std::string error_str;
    llvm::raw_string_ostream error_stream(error_str);
    if (llvm::verifyModule(*module_, &error_stream)) {
        std::cerr << "Module verification failed:\n" << error_str << std::endl;
        return false;
    }

    // std::cerr << ">>> [6] Done!\n";
    return true;
}


void LLVMCodeGen::codegen_field_assignment(aithon::parser::ast::FieldAssignment* stmt) {
    using namespace aithon::parser::ast;

    llvm::Value* new_val = codegen_expr(stmt->value.get());
    if (!new_val) return;

    auto* ident = dynamic_cast<Identifier*>(stmt->object.get());
    if (!ident) {
        std::cerr << "ERROR: Field assignment only supported on variables\n";
        return;
    }

    auto it = variables_.find(ident->name);
    if (it == variables_.end()) {
        std::cerr << "ERROR: Unknown variable: " << ident->name << "\n";
        return;
    }

    llvm::AllocaInst* obj_ptr = it->second.alloca;
    std::string type_name = it->second.type_name;  // ✅ Get tracked type name

    if (type_name.empty()) {
        std::cerr << "ERROR: No type information for variable\n";
        return;
    }

    // Check if struct
    auto struct_it = struct_types_.find(type_name);
    if (struct_it != struct_types_.end()) {
        codegen_struct_field_assign(obj_ptr, type_name, stmt->field_name, new_val);
        return;
    }

    // Check if class
    auto class_it = class_types_.find(type_name);
    if (class_it != class_types_.end()) {
        llvm::Value* obj = builder_->CreateLoad(it->second.type, obj_ptr);
        codegen_class_field_assign(obj, type_name, stmt->field_name, new_val);
        return;
    }

    std::cerr << "ERROR: Unknown type: " << type_name << "\n";
    }

    void LLVMCodeGen::codegen_index_assignment(parser::ast::IndexAssignment* stmt) {
        using namespace aithon::parser::ast;

        // Generate the object (list/dict)
        llvm::Value* obj = codegen_expr(stmt->object.get());
        if (!obj) return;

        // Generate the index
        llvm::Value* index = codegen_expr(stmt->index.get());
        if (!index) return;

        // Generate the value
        llvm::Value* value = codegen_expr(stmt->value.get());
        if (!value) return;

        // Determine if this is a list or dict
        // For now, assume list with integer index
        llvm::Function* set_fn = module_->getFunction("runtime_list_set_int");
        if (!set_fn) {
            std::cerr << "ERROR: runtime_list_set_int not declared\n";
            return;
        }

        builder_->CreateCall(set_fn, {obj, index, value});
}


// ============================================================================
// Statement Code Generation
// ============================================================================

void LLVMCodeGen::codegen_stmt(parser::ast::Stmt* stmt) {
    if (!stmt) return;

    if (auto* func = dynamic_cast<parser::ast::FunctionDecl*>(stmt)) {
        codegen_function(func);
    } else if (auto* block = dynamic_cast<parser::ast::Block*>(stmt)) {
        codegen_block(block);
    } else if (auto* if_stmt = dynamic_cast<parser::ast::IfStmt*>(stmt)) {
        codegen_if(if_stmt);
    } else if (auto* while_stmt = dynamic_cast<parser::ast::WhileStmt*>(stmt)) {
        codegen_while(while_stmt);
    } else if (auto* for_stmt = dynamic_cast<parser::ast::ForStmt*>(stmt)) {
        codegen_for(for_stmt);
    } else if (auto* ret = dynamic_cast<parser::ast::ReturnStmt*>(stmt)) {
        codegen_return(ret);
    } else if (auto* assign = dynamic_cast<parser::ast::Assignment*>(stmt)) {
        codegen_assignment(assign);
    } else if (auto* expr_stmt = dynamic_cast<parser::ast::ExprStmt*>(stmt)) {
        codegen_expr_stmt(expr_stmt);
    } else if (auto* s = dynamic_cast<parser::ast::StructDecl*>(stmt)) {
        codegen_struct_decl(s);
    } else if (auto* c = dynamic_cast<parser::ast::ClassDecl*>(stmt)) {
        codegen_class_decl(c);
    } else if (auto* field_assign = dynamic_cast<parser::ast::FieldAssignment*>(stmt)) {
        codegen_field_assignment(field_assign);
    } else if (auto* index_assign = dynamic_cast<parser::ast::IndexAssignment*>(stmt)) {
        codegen_index_assignment(index_assign);
    }
}

void LLVMCodeGen::codegen_function(parser::ast::FunctionDecl* func) {
    // Create function name (rename main to python_main)
    std::string func_name = func->name;
    if (func->name == "main") {
        func_name = "python_main";
    }

    // Create function type (all parameters are i64 for simplicity)
    std::vector<llvm::Type*> param_types(func->parameters.size(),
                                         llvm::Type::getInt64Ty(*context_));
    llvm::FunctionType* func_type = llvm::FunctionType::get(
        llvm::Type::getInt64Ty(*context_),
        param_types,
        false
    );

    // Create function
    llvm::Function* llvm_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        func_name,
        module_.get()
    );

    // Store function
    functions_[func->name] = llvm_func;
    if (func_name != func->name) {
        functions_[func_name] = llvm_func;
    }
    current_function_ = llvm_func;

    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", llvm_func);
    builder_->SetInsertPoint(entry);

    // Create allocas for parameters
    size_t idx = 0;
    for (auto& arg : llvm_func->args()) {
        std::string param_name = func->parameters[idx++].name;
        llvm::AllocaInst* alloca = create_entry_block_alloca(llvm_func, param_name);
        builder_->CreateStore(&arg, alloca);
        named_values_[param_name] = alloca;
        variables_[param_name].alloca;

    }

    // Generate function body
    codegen_block(func->body.get());

    // Add return if missing
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateRet(llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)));
    }

    // Verify function
    std::string error_str;
    llvm::raw_string_ostream error_stream(error_str);
    if (llvm::verifyFunction(*llvm_func, &error_stream)) {
        std::cerr << "Function verification failed: " << func_name << "\n"
                  << error_str << std::endl;
    }

    // Optimize
    if (fpm_) {
        fpm_->run(*llvm_func);
    }

    // Clear local variables
    variables_.clear();
    named_values_.clear();
    var_types_.clear();          // ← ADD
    var_llvm_types_.clear();     // ← ADD
    current_function_ = nullptr;

    // Release all class objects before function exit
    for (auto& [name, alloca] : named_values_) {
        auto type_it = var_llvm_types_.find(name);
        if (type_it != var_llvm_types_.end() && type_it->second->isPointerTy()) {
            llvm::Value* val = builder_->CreateLoad(type_it->second, alloca);
            llvm::Function* release_fn = module_->getFunction("runtime_release");
            if (release_fn) {
                builder_->CreateCall(release_fn, {val});
            }
        }
    }

}

void LLVMCodeGen::codegen_block(parser::ast::Block* block) {
    for (auto& stmt : block->statements) {
        codegen_stmt(stmt.get());

        // Stop if we hit a terminator
        if (builder_->GetInsertBlock()->getTerminator()) {
            break;
        }
    }
}

void LLVMCodeGen::codegen_if(parser::ast::IfStmt* stmt) {
    llvm::Value* cond = codegen_expr(stmt->condition.get());
    if (!cond) return;

    // Convert condition to boolean
    cond = builder_->CreateICmpNE(
        cond,
        llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)),
        "ifcond"
    );

    llvm::Function* func = builder_->GetInsertBlock()->getParent();

    // Create blocks
    llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*context_, "then", func);
    llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(*context_, "else");
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*context_, "ifcont");

    builder_->CreateCondBr(cond, then_bb, else_bb);

    // Then block
    builder_->SetInsertPoint(then_bb);
    codegen_block(stmt->then_block.get());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(merge_bb);
    }

    // Else block
    func->insert(func->end(), else_bb);
    builder_->SetInsertPoint(else_bb);
    if (stmt->else_block) {
        codegen_block(stmt->else_block.get());
    }
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(merge_bb);
    }

    // Merge block
    func->insert(func->end(), merge_bb);
    builder_->SetInsertPoint(merge_bb);
}

void LLVMCodeGen::codegen_while(parser::ast::WhileStmt* stmt) {
    llvm::Function* func = builder_->GetInsertBlock()->getParent();

    llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(*context_, "whilecond", func);
    llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(*context_, "whilebody", func);
    llvm::BasicBlock* end_bb = llvm::BasicBlock::Create(*context_, "whileend", func);

    builder_->CreateBr(cond_bb);

    // Condition block
    builder_->SetInsertPoint(cond_bb);
    llvm::Value* cond = codegen_expr(stmt->condition.get());
    cond = builder_->CreateICmpNE(
        cond,
        llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)),
        "whilecond"
    );
    builder_->CreateCondBr(cond, body_bb, end_bb);

    // Body block
    builder_->SetInsertPoint(body_bb);
    codegen_block(stmt->body.get());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(cond_bb);
    }

    // End block
    builder_->SetInsertPoint(end_bb);
}

void LLVMCodeGen::codegen_for(parser::ast::ForStmt* stmt) {
    // For now, simplified for loop
    // In full implementation, would handle iterables properly
    std::cerr << "Warning: For loops not fully implemented yet\n";
}

void LLVMCodeGen::codegen_return(parser::ast::ReturnStmt* stmt) {
    if (stmt->value) {
        llvm::Value* ret_val = codegen_expr(stmt->value.get());
        if (ret_val) {
            builder_->CreateRet(ret_val);
        }
    } else {
        builder_->CreateRet(llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)));
    }
}



llvm::Value* LLVMCodeGen::codegen_initializer(aithon::parser::ast::InitializerExpr* expr) {
    using namespace aithon::parser::ast;

    // Check if it's a struct
    auto struct_it = struct_types_.find(expr->type_name);
    if (struct_it != struct_types_.end()) {
        // It's a struct initializer
        StructInfo& info = struct_it->second;

        // Build field values in the correct order
        std::vector<llvm::Value*> field_values;

        for (size_t i = 0; i < info.field_names.size(); ++i) {
            const std::string& field_name = info.field_names[i];

            // Find the named argument for this field
            llvm::Value* field_val = nullptr;
            for (auto& arg : expr->arguments) {
                if (arg.name == field_name) {
                    field_val = codegen_expr(arg.value.get());
                    break;
                }
            }

            // If not provided, use default value (0 for now)
            if (!field_val) {
                // TODO: Use actual default value from FieldDecl
                switch (info.field_types[i]) {
                    case VarType::INT:
                        field_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0);
                        break;
                    case VarType::FLOAT:
                        field_val = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context_), 0.0);
                        break;
                    case VarType::BOOL:
                        field_val = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context_), 0);
                        break;
                    default:
                        field_val = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context_));
                        break;
                }
            }

            field_values.push_back(field_val);
        }

        // Create the struct instance
        return codegen_struct_init(expr->type_name, field_values);
    }

    // Check if it's a class
    auto class_it = class_types_.find(expr->type_name);
    if (class_it != class_types_.end()) {
        // It's a class initializer
        ClassInfo& info = class_it->second;

        // Build field values in the correct order
        std::vector<llvm::Value*> field_values;

        for (size_t i = 0; i < info.field_names.size(); ++i) {
            const std::string& field_name = info.field_names[i];

            // Find the named argument for this field
            llvm::Value* field_val = nullptr;
            for (auto& arg : expr->arguments) {
                if (arg.name == field_name) {
                    field_val = codegen_expr(arg.value.get());
                    break;
                }
            }

            // If not provided, use default value
            if (!field_val) {
                switch (info.field_types[i]) {
                    case VarType::INT:
                        field_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0);
                        break;
                    case VarType::FLOAT:
                        field_val = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context_), 0.0);
                        break;
                    case VarType::BOOL:
                        field_val = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context_), 0);
                        break;
                    default:
                        field_val = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context_));
                        break;
                }
            }

            field_values.push_back(field_val);
        }

        // Create the class instance
        return codegen_class_init(expr->type_name, field_values);
    }

    std::cerr << "ERROR: Unknown type for initializer: " << expr->type_name << "\n";
    return nullptr;
}


llvm::Value* LLVMCodeGen::codegen_member_access(parser::ast::MemberExpr* expr) {
    using namespace aithon::parser::ast;

    // Get the object
    auto* ident = dynamic_cast<Identifier*>(expr->object.get());
    if (!ident) {
        std::cerr << "ERROR: Member access only supported on variables\n";
        return nullptr;
    }

    auto it = variables_.find(ident->name);
    if (it == variables_.end()) {
        std::cerr << "ERROR: Unknown variable: " << ident->name << "\n";
        return nullptr;
    }

    std::string type_name = it->second.type_name;
    if (type_name.empty()) {
        std::cerr << "ERROR: No type info for variable\n";
        return nullptr;
    }

    // Check if struct
    auto struct_it = struct_types_.find(type_name);
    if (struct_it != struct_types_.end()) {
        // Load struct value
        llvm::Value* struct_val = builder_->CreateLoad(it->second.type, it->second.alloca);
        return codegen_struct_field_access(struct_val, type_name, expr->member);
    }

    // Check if class
    auto class_it = class_types_.find(type_name);
    if (class_it != class_types_.end()) {
        // Load object pointer
        llvm::Value* obj_ptr = builder_->CreateLoad(it->second.type, it->second.alloca);
        return codegen_class_field_access(obj_ptr, type_name, expr->member);
    }

    std::cerr << "ERROR: Unknown type: " << type_name << "\n";
    return nullptr;
}


// Identifier
/*
llvm::Value* LLVMCodeGen::codegen_identifier(parser::ast::Identifier* expr) {
    auto it = variables_.find(expr->name);
    if (it == variables_.end()) {
        // error("Unknown variable: " + expr->name);
        return nullptr;
    }

    // Use the tracked type
    return builder_->CreateLoad(it->second.type, it->second.alloca);  // ✅
}
*/

void LLVMCodeGen::codegen_expr_stmt(parser::ast::ExprStmt* stmt) {
    codegen_expr(stmt->expression.get());
}

// ============================================================================
// Expression Code Generation
// ============================================================================

llvm::Value* LLVMCodeGen::codegen_expr(parser::ast::Expr* expr) {
    if (!expr) return nullptr;

    static int call_depth = 0;
    ++call_depth;

    std::string indent(call_depth * 2, ' ');
    std::cerr << indent << ">>> codegen_expr[" << call_depth << "] ENTER\n";

    if (!expr) {
        std::cerr << indent << ">>> codegen_expr[" << call_depth << "] expr is NULL\n";
        --call_depth;
        return nullptr;
    }

    // ... rest of your type checks ...

    if (auto* int_lit = dynamic_cast<parser::ast::IntegerLiteral*>(expr)) return codegen_integer(int_lit);
    if (auto* float_lit = dynamic_cast<parser::ast::FloatLiteral*>(expr)) return codegen_float(float_lit);
    if (auto* str_lit = dynamic_cast<parser::ast::StringLiteral*>(expr)) return codegen_string(str_lit);
    if (auto* bool_lit = dynamic_cast<parser::ast::BoolLiteral*>(expr)) return codegen_bool(bool_lit);
    if (auto* none_lit = dynamic_cast<parser::ast::NoneLiteral*>(expr)) return codegen_none(none_lit);
    if (auto* ident = dynamic_cast<parser::ast::Identifier*>(expr)) return codegen_identifier(ident);
    if (auto* binop = dynamic_cast<parser::ast::BinaryOp*>(expr)) return codegen_binary_op(binop);
    if (auto* unop = dynamic_cast<parser::ast::UnaryOp*>(expr)) return codegen_unary_op(unop);
    if (auto* call = dynamic_cast<parser::ast::CallExpr*>(expr)) return codegen_call(call);

    if (auto* list = dynamic_cast<parser::ast::ListExpr*>(expr)) return codegen_list(list);
    if (auto* dict = dynamic_cast<parser::ast::DictExpr*>(expr)) return codegen_dict(dict);
    if (auto* index = dynamic_cast<parser::ast::IndexExpr*>(expr)) return codegen_index(index);


    // ✅ ADD THIS
    // if (auto* e = dynamic_cast<parser::ast::InitializerExpr*>(expr))   return codegen_initializer(e);

if (auto* e = dynamic_cast<parser::ast::InitializerExpr*>(expr)) {
    std::cerr << "  -> InitializerExpr (converting to function call)\n";
    std::cerr << "     Type name: " << e->type_name << "\n";

    // Look up the constructor function
    auto it = functions_.find(e->type_name);
    if (it == functions_.end()) {
        std::cerr << "ERROR: Constructor not found: " << e->type_name << "\n";
        std::cerr << "Available functions:\n";
        for (const auto& [name, fn] : functions_) {
            std::cerr << "  - " << name << "\n";
        }
        return nullptr;
    }

    llvm::Function* fn = it->second;
    std::cerr << "     Found constructor function\n";

    // Generate arguments in field order
    // First, get the struct/class info to know field order
    auto struct_it = struct_types_.find(e->type_name);
    if (struct_it == struct_types_.end()) {
        std::cerr << "ERROR: Unknown struct type: " << e->type_name << "\n";
        return nullptr;
    }

    StructInfo& info = struct_it->second;
    std::cerr << "     Generating " << info.field_names.size() << " arguments\n";

    // Build arguments in the correct field order
    std::vector<llvm::Value*> args;
    for (size_t i = 0; i < info.field_names.size(); ++i) {
        const std::string& field_name = info.field_names[i];

        // Find the named argument for this field
        llvm::Value* arg_val = nullptr;
        for (auto& named_arg : e->arguments) {
            if (named_arg.name == field_name) {
                std::cerr << "     Field '" << field_name << "': generating...\n";
                arg_val = codegen_expr(named_arg.value.get());
                if (!arg_val) {
                    std::cerr << "ERROR: Failed to generate value for field: " << field_name << "\n";
                    return nullptr;
                }
                std::cerr << "     Field '" << field_name << "': OK\n";
                break;
            }
        }

        if (!arg_val) {
            std::cerr << "ERROR: No value provided for field: " << field_name << "\n";
            return nullptr;
        }

        args.push_back(arg_val);
    }

    std::cerr << "     Calling constructor with " << args.size() << " args\n";
    llvm::Value* result = builder_->CreateCall(fn, args);

    if (result) {
        std::cerr << "     Constructor call SUCCESS\n";
    } else {
        std::cerr << "     Constructor call returned NULL\n";
    }

    return result;
}

    // At the end, before returning:
    std::cerr << indent << ">>> codegen_expr[" << call_depth << "] EXIT\n";
    --call_depth;
    // return result;  // whatever you're returning
    return nullptr;
}

llvm::Value* LLVMCodeGen::codegen_integer(const parser::ast::IntegerLiteral* expr) const {
    return llvm::ConstantInt::get(*context_, llvm::APInt(64, expr->value, true));
}

llvm::Value* LLVMCodeGen::codegen_float(const parser::ast::FloatLiteral* expr) const {
    return llvm::ConstantFP::get(*context_, llvm::APFloat(expr->value));
}

llvm::Value* LLVMCodeGen::codegen_string(const parser::ast::StringLiteral* expr) const {
    return builder_->CreateGlobalString(expr->value);
}

llvm::Value* LLVMCodeGen::codegen_bool(const parser::ast::BoolLiteral* expr) const {
    return llvm::ConstantInt::get(*context_, llvm::APInt(1, expr->value ? 1 : 0));
}

llvm::Value* LLVMCodeGen::codegen_none(parser::ast::NoneLiteral* expr) const {
    return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
}

llvm::Value* LLVMCodeGen::codegen_identifier(const parser::ast::Identifier* expr) {
    // llvm::Value* var = named_values_[expr->name];
    llvm::Value* var = variables_[expr->name].alloca;

    if (!var) {
        std::cerr << "Unknown variable: " << expr->name << std::endl;
        return nullptr;
    }

    return builder_->CreateLoad(
        llvm::Type::getInt64Ty(*context_),
        var,
        expr->name
    );
}

llvm::Value* LLVMCodeGen::codegen_binary_op(const parser::ast::BinaryOp* expr) {
    llvm::Value* left = codegen_expr(expr->left.get());
    llvm::Value* right = codegen_expr(expr->right.get());

    if (!left || !right) return nullptr;

    switch (expr->op) {
        case parser::ast::BinaryOp::Op::ADD:
            return builder_->CreateAdd(left, right, "addtmp");
        case parser::ast::BinaryOp::Op::SUB:
            return builder_->CreateSub(left, right, "subtmp");
        case parser::ast::BinaryOp::Op::MUL:
            return builder_->CreateMul(left, right, "multmp");
        case parser::ast::BinaryOp::Op::DIV:
            return builder_->CreateSDiv(left, right, "divtmp");
        case parser::ast::BinaryOp::Op::MOD:
            return builder_->CreateSRem(left, right, "modtmp");
        case parser::ast::BinaryOp::Op::LESS:
            return builder_->CreateICmpSLT(left, right, "cmptmp");
        case parser::ast::BinaryOp::Op::LESS_EQUAL:
            return builder_->CreateICmpSLE(left, right, "cmptmp");
        case parser::ast::BinaryOp::Op::GREATER:
            return builder_->CreateICmpSGT(left, right, "cmptmp");
        case parser::ast::BinaryOp::Op::GREATER_EQUAL:
            return builder_->CreateICmpSGE(left, right, "cmptmp");
        case parser::ast::BinaryOp::Op::EQUAL:
            return builder_->CreateICmpEQ(left, right, "cmptmp");
        case parser::ast::BinaryOp::Op::NOT_EQUAL:
            return builder_->CreateICmpNE(left, right, "cmptmp");
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::codegen_unary_op(const parser::ast::UnaryOp* expr) {
    llvm::Value* operand = codegen_expr(expr->operand.get());
    if (!operand) return nullptr;

    switch (expr->op) {
        case parser::ast::UnaryOp::Op::NEG:
            return builder_->CreateNeg(operand, "negtmp");
        case parser::ast::UnaryOp::Op::NOT:
            return builder_->CreateNot(operand, "nottmp");
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::codegen_call(parser::ast::CallExpr *expr) {
    // Get function name
    auto* callee_ident = dynamic_cast<parser::ast::Identifier*>(expr->callee.get());
    if (!callee_ident) {
        std::cerr << "Only simple function calls supported\n";
        return nullptr;
    }

    std::string func_name = callee_ident->name;


    if (func_name == "print") return codegen_print_call(expr);  // ← routes here

    // Special handling for print
    if (func_name == "print") {
        for (auto& arg : expr->arguments) {
            llvm::Value* val = codegen_expr(arg.get());
            if (!val) continue;

            llvm::Function* print_fn = nullptr;
            auto it = variables_.find(callee_ident->name);
            if (it != variables_.end()) {
                switch (it->second.var_type) {  // ← Use var_type field
                    case VarType::LIST:
                        print_fn = module_->getFunction("runtime_list_print");
                        break;
                    case VarType::DICT:
                        print_fn = module_->getFunction("runtime_dict_print");
                        break;
                    case VarType::INT:
                        print_fn = module_->getFunction("runtime_print_int");
                        break;

                    case VarType::STRING:
                        print_fn = module_->getFunction("runtime_print_string");
                        break;
                    default:
                        print_fn = module_->getFunction("runtime_print_string");
                }

                builder_->CreateCall(print_fn, {val});
            }








            // // Check what type of expression this is
            // if (dynamic_cast<parser::ast::ListExpr*>(arg.get())) {
            //     llvm::Function* print_list = module_->getFunction("runtime_list_print");
            //     builder_->CreateCall(print_list, {val});
            // } else if (dynamic_cast<parser::ast::DictExpr*>(arg.get())) {
            //     llvm::Function* print_dict = module_->getFunction("runtime_dict_print");
            //     builder_->CreateCall(print_dict, {val});
            // }
            // else if (val->getType()->isIntegerTy(64)) {
            //     llvm::Function* print_int = module_->getFunction("runtime_print_int");
            //     builder_->CreateCall(print_int, {val});
            // } else if (val->getType()->isDoubleTy()) {
            //     llvm::Function* print_float = module_->getFunction("runtime_print_float");
            //     builder_->CreateCall(print_float, {val});
            // } else if (val->getType()->isPointerTy()) {
            //     llvm::Function* print_str = module_->getFunction("runtime_print_string");
            //     builder_->CreateCall(print_str, {val});
            // }
        }
        return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
    }

    // Regular function call
    llvm::Function* callee = functions_[func_name];
    if (!callee) {
        std::cerr << "Unknown function: " << func_name << std::endl;
        return nullptr;
    }

    // Generate arguments
    std::vector<llvm::Value*> args;
    for (auto& arg : expr->arguments) {
        llvm::Value* val = codegen_expr(arg.get());
        if (val) {
            args.push_back(val);
        }
    }

    return builder_->CreateCall(callee, args, "calltmp");
}


// ============================================================================
// Main Wrapper Generation
// ============================================================================

llvm::Function* LLVMCodeGen::generate_main_wrapper() const {
    // Create C main function: int main(int argc, char** argv)
    llvm::Type* i32 = llvm::Type::getInt32Ty(*context_);
    llvm::Type* ptr = llvm::PointerType::getUnqual(*context_);

    llvm::FunctionType* main_type = llvm::FunctionType::get(
        i32,
        {i32, ptr},
        false
    );

    llvm::Function* main_func = llvm::Function::Create(
        main_type,
        llvm::Function::ExternalLinkage,
        "main",
        module_.get()
    );

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", main_func);
    builder_->SetInsertPoint(entry);

    // Call python_main()
    llvm::Function* python_main = module_->getFunction("python_main");
    if (python_main) {
        llvm::Value* result = builder_->CreateCall(python_main);
        llvm::Value* i32_result = builder_->CreateTrunc(result, i32);
        builder_->CreateRet(i32_result);
    } else {
        builder_->CreateRet(llvm::ConstantInt::get(i32, 0));
    }

    return main_func;
}

// ============================================================================
// Output Methods
// ============================================================================

void LLVMCodeGen::dump_ir() const {
    module_->print(llvm::outs(), nullptr);
}

bool LLVMCodeGen::write_ir_to_file(const std::string& filename) const {
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << std::endl;
        return false;
    }

    module_->print(dest, nullptr);
    return true;
}

bool LLVMCodeGen::write_object_to_file(const std::string& filename) const {
    // Use LLVM's default target triple (built-in macro)
    const std::string target_triple = LLVM_DEFAULT_TARGET_TRIPLE;

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        std::cerr << "Target lookup failed: " << error << std::endl;
        return false;
    }

    const llvm::TargetOptions opt;
    auto rm = llvm::Reloc::Model();
    auto target_machine = target->createTargetMachine(
        target_triple, "generic", "", opt, rm);

    module_->setDataLayout(target_machine->createDataLayout());
    module_->setTargetTriple(llvm::Triple(target_triple));


    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << std::endl;
        return false;
    }

    llvm::legacy::PassManager pass;
    auto file_type = llvm::CodeGenFileType::ObjectFile;
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
        return false;
    }

    pass.run(*module_);
    dest.flush();

    return true;
}

void LLVMCodeGen::optimize() const {
    if (fpm_) {
        for (auto& func : *module_) {
            if (!func.isDeclaration()) {
                fpm_->run(func);
            }
        }
    }
}


// ============================================================================
// Code Generation for Dictionary and List Literals
// ============================================================================

llvm::Value* LLVMCodeGen::codegen_dict(parser::ast::DictExpr* expr) {
    // Create a new dictionary
    llvm::Function* create_fn = module_->getFunction("runtime_dict_create");
    llvm::Value* dict_ptr = builder_->CreateCall(create_fn);

    // Set each key-value pair
    for (auto& [key_expr, val_expr] : expr->pairs) {
        // Key must be a string literal
        auto* key_str = dynamic_cast<parser::ast::StringLiteral*>(key_expr.get());
        if (!key_str) {
            std::cerr << "Warning: Dictionary keys must be strings\n";
            continue;
        }

        llvm::Value* key = builder_->CreateGlobalString(key_str->value);
        llvm::Value* val = codegen_expr(val_expr.get());

        if (!val) continue;

        // Check value type and call appropriate set function
        if (val->getType()->isIntegerTy(64)) {
            llvm::Function* set_fn = module_->getFunction("runtime_dict_set_int");
            builder_->CreateCall(set_fn, {dict_ptr, key, val});
        }
        else if (val->getType()->isPointerTy()) {
            // Assume it's a string
            llvm::Function* set_fn = module_->getFunction("runtime_dict_set_string");
            builder_->CreateCall(set_fn, {dict_ptr, key, val});
        }
    }

    return dict_ptr;
}

// ============================================================================
// Code Generation for Index Access (list[0] or dict["key"])
// ============================================================================

llvm::Value* LLVMCodeGen::codegen_index(parser::ast::IndexExpr* expr) {
    // Generate the object being indexed
    llvm::Value* obj = codegen_expr(expr->object.get());
    if (!obj) return nullptr;

    // Check if index is an integer (list) or string (dict)
    llvm::Value* index = codegen_expr(expr->index.get());
    if (!index) return nullptr;

    if (index->getType()->isIntegerTy(64)) {
        // List access: list[index]
        llvm::Function* get_fn = module_->getFunction("runtime_list_get_string");
        return builder_->CreateCall(get_fn, {obj, index});
    }
    else if (index->getType()->isPointerTy()) {
        // Dictionary access: dict["key"]
        llvm::Function* get_fn = module_->getFunction("runtime_dict_get_string");
        return builder_->CreateCall(get_fn, {obj, index});
    }

    return nullptr;
}

llvm::Value* LLVMCodeGen::codegen_list(parser::ast::ListExpr* expr) {
    // Create a new list
    llvm::Function* create_fn = module_->getFunction("runtime_list_create");
    llvm::Value* list_ptr = builder_->CreateCall(create_fn);

    // Append each element
    for (auto& elem : expr->elements) {
        llvm::Value* val = codegen_expr(elem.get());
        if (!val) continue;

        // Check type and call appropriate append function
        if (val->getType()->isIntegerTy(64)) {
            llvm::Function* append_fn = module_->getFunction("runtime_list_append_int");
            builder_->CreateCall(append_fn, {list_ptr, val});
        }
        else if (val->getType()->isPointerTy()) {
            // Assume it's a string
            llvm::Function* append_fn = module_->getFunction("runtime_list_append_string");
            builder_->CreateCall(append_fn, {list_ptr, val});
        }
    }

    return list_ptr;
}

// ============================================================================
// Step 1: Add this helper to infer VarType from an expression
// ============================================================================

LLVMCodeGen::VarType LLVMCodeGen::infer_var_type(parser::ast::Expr* expr) {
    if (!expr) return VarType::UNKNOWN;

    if (dynamic_cast<parser::ast::ListExpr*>(expr)) {
        return VarType::LIST;
    }
    if (dynamic_cast<parser::ast::DictExpr*>(expr)) {
        return VarType::DICT;
    }
    if (dynamic_cast<parser::ast::StringLiteral*>(expr)) {
        return VarType::STRING;
    }
    if (dynamic_cast<parser::ast::IntegerLiteral*>(expr)) {
        return VarType::INT;
    }
    if (dynamic_cast<parser::ast::FloatLiteral*>(expr)) {
        return VarType::FLOAT;
    }
    if (dynamic_cast<parser::ast::BoolLiteral*>(expr)) {
        return VarType::BOOL;
    }
    if (auto* ident = dynamic_cast<parser::ast::Identifier*>(expr)) {
        // Propagate type from existing variable
        auto it = variables_.find(ident->name);
        if (it != variables_.end()) {
            return it->second.var_type;
        }
    }
    return VarType::UNKNOWN;
}

    // ============================================================================
    // STEP 3: get_print_fn_for() - add to llvm_codegen.cpp
    // ============================================================================

    llvm::Function*
    LLVMCodeGen::get_print_fn_for(VarType vt, llvm::Value* val) {
    switch (vt) {
        case VarType::INT:    return module_->getFunction("runtime_print_int");
        case VarType::FLOAT:  return module_->getFunction("runtime_print_float");
        case VarType::BOOL:   return module_->getFunction("runtime_print_bool");
        case VarType::STRING: return module_->getFunction("runtime_print_string");
        case VarType::LIST:   return module_->getFunction("runtime_list_print");
        case VarType::DICT:   return module_->getFunction("runtime_dict_print");
        default: break;
    }

    // Fallback: infer from LLVM type
    if (!val) return nullptr;
    llvm::Type* t = val->getType();
    if (t->isIntegerTy(64)) return module_->getFunction("runtime_print_int");
    if (t->isDoubleTy())    return module_->getFunction("runtime_print_float");
    if (t->isIntegerTy(1))  return module_->getFunction("runtime_print_bool");
    if (t->isPointerTy())   return module_->getFunction("runtime_print_string");

    return nullptr;
}

// ============================================================================
// Step 2: Complete working codegen_assignment
// ============================================================================

void LLVMCodeGen::codegen_assignment(aithon::parser::ast::Assignment* stmt) {
    std::cerr << ">>> codegen_assignment: " << stmt->name << "\n";

    llvm::Value* value = codegen_expr(stmt->value.get());

    if (!value) {
        std::cerr << "ERROR: codegen_assignment: codegen_expr returned NULL for '"
                  << stmt->name << "'\n";
        return;
    }

    std::cerr << ">>> codegen_assignment: Got value successfully\n";

    if (!current_function_) {
        std::cerr << "ERROR: Assignment outside function for '" << stmt->name << "'\n";
        return;
    }

    std::cerr << ">>> codegen_assignment: Inside function, creating variable\n";

    // Infer semantic type
    VarType vt = infer_var_type(stmt->value.get());
    std::cerr << ">>> Inferred var_type: " << static_cast<int>(vt) << "\n";

    // Track type name
    std::string type_name;
    if (auto* init_expr = dynamic_cast<aithon::parser::ast::InitializerExpr*>(stmt->value.get())) {
        type_name = init_expr->type_name;
        std::cerr << ">>> Type name: " << type_name << "\n";
    }

    // Store type info
    var_types_[stmt->name] = vt;
    var_llvm_types_[stmt->name] = value->getType();
    std::cerr << ">>> Stored in var_types_ and var_llvm_types_\n";

    auto it = variables_.find(stmt->name);

    if (it == variables_.end()) {
        std::cerr << ">>> NEW VARIABLE\n";

        llvm::Type* storage_type = value->getType();
        std::cerr << ">>> Storage type obtained\n";

        llvm::IRBuilder<> entry_builder(
            &current_function_->getEntryBlock(),
            current_function_->getEntryBlock().begin()
        );
        std::cerr << ">>> Entry builder created\n";

        llvm::AllocaInst* var_alloca = entry_builder.CreateAlloca(
            storage_type, nullptr, stmt->name
        );
        std::cerr << ">>> Alloca created\n";

        builder_->CreateStore(value, var_alloca);
        std::cerr << ">>> Value stored\n";

        // Create VarInfo
        VarInfo info;
        info.alloca = var_alloca;
        info.type = storage_type;
        info.var_type = vt;
        info.type_name = type_name;

        variables_[stmt->name] = info;
        std::cerr << ">>> Variable '" << stmt->name << "' stored in variables_\n";

        // Verify it was stored
        auto verify = variables_.find(stmt->name);
        if (verify != variables_.end()) {
            std::cerr << ">>> VERIFIED: '" << stmt->name << "' is in variables_\n";
        } else {
            std::cerr << ">>> ERROR: '" << stmt->name << "' NOT in variables_!\n";
        }

    } else {
        std::cerr << ">>> EXISTING VARIABLE (reassignment)\n";
        // ... existing variable code ...
    }

    std::cerr << ">>> codegen_assignment: DONE\n";
}
    // ============================================================================
    // STEP 5: Update codegen_identifier() - use var_llvm_types_ for load type
    // ============================================================================

    llvm::Value* LLVMCodeGen::codegen_identifier(parser::ast::Identifier* expr) {
    auto it = named_values_.find(expr->name);
    if (it == named_values_.end()) {
        std::cerr << "ERROR: Unknown variable: " << expr->name << "\n";
        return nullptr;
    }

    // Look up the stored LLVM type (needed for opaque pointers in LLVM 15+)
    auto ty_it = var_llvm_types_.find(expr->name);
    if (ty_it == var_llvm_types_.end()) {
        std::cerr << "ERROR: No type info for: " << expr->name << "\n";
        return nullptr;
    }

    return builder_->CreateLoad(ty_it->second, it->second, expr->name);
}
// ============================================================================
// Step 4: Helper — pick the right print function from a VarType
// ============================================================================

llvm::Function* LLVMCodeGen::get_print_function(VarType var_type,
                                                  llvm::Value* value) {
    switch (var_type) {
        case VarType::INT:
            return module_->getFunction("runtime_print_int");
        case VarType::FLOAT:
            return module_->getFunction("runtime_print_float");
        case VarType::BOOL:
            return module_->getFunction("runtime_print_bool");
        case VarType::STRING:
            return module_->getFunction("runtime_print_string");
        case VarType::LIST:
            return module_->getFunction("runtime_list_print");
        case VarType::DICT:
            return module_->getFunction("runtime_dict_print");
        default:
            break;
    }

    // Last-resort fallback: infer from LLVM type
    if (value->getType()->isIntegerTy(64)) {
        return module_->getFunction("runtime_print_int");
    }
    if (value->getType()->isDoubleTy()) {
        return module_->getFunction("runtime_print_float");
    }
    if (value->getType()->isIntegerTy(1)) {
        return module_->getFunction("runtime_print_bool");
    }
    if (value->getType()->isPointerTy()) {
        return module_->getFunction("runtime_print_string");
    }

    return nullptr;
}

// ============================================================================
// Step 5: Complete working codegen_print
// ============================================================================
/*
void LLVMCodeGen::codegen_print(parser::ast::PrintStmt* stmt) {
    for (auto& arg : stmt->arguments) {

        // ── 1. Determine semantic type BEFORE codegen ────────────────────────
        VarType var_type = VarType::UNKNOWN;

        if (auto* ident = dynamic_cast<parser::ast::Identifier*>(arg.get())) {
            // Variable — look up its type
            auto it = variables_.find(ident->name);
            if (it != variables_.end()) {
                var_type = it->second.var_type;
            } else {
                std::cerr << "WARNING: print: unknown variable '"
                          << ident->name << "'\n";
            }
        } else {
            // Literal / expression — infer directly
            var_type = infer_var_type(arg.get());
        }

        // ── 2. Generate the value ────────────────────────────────────────────
        llvm::Value* value = codegen_expr(arg.get());
        if (!value) {
            std::cerr << "WARNING: print: codegen_expr returned null\n";
            continue;
        }

        // ── 3. Pick print function ───────────────────────────────────────────
        llvm::Function* print_fn = get_print_function(var_type, value);

        if (!print_fn) {
            std::cerr << "ERROR: print: no print function found for var_type="
                      << static_cast<int>(var_type) << "\n";
            continue;
        }

        // ── 4. Call it ───────────────────────────────────────────────────────
        builder_->CreateCall(print_fn, {value});
    }
}
*/

llvm::Value* LLVMCodeGen::codegen_print_call(parser::ast::CallExpr* expr) {
    for (auto& arg_expr : expr->arguments) {

        // 1. Resolve semantic type BEFORE generating the value
        VarType vt = VarType::UNKNOWN;

        if (auto* ident = dynamic_cast<parser::ast::Identifier*>(arg_expr.get())) {
            auto it = var_types_.find(ident->name);
            if (it != var_types_.end())
                vt = it->second;
        } else {
            vt = infer_var_type(arg_expr.get());
        }

        // 2. Generate the value
        llvm::Value* val = codegen_expr(arg_expr.get());
        if (!val) {
            std::cerr << "WARNING: print arg codegen returned null\n";
            continue;
        }

        // 3. Pick the right print function
        llvm::Function* print_fn = get_print_fn_for(vt, val);

        if (!print_fn) {
            std::cerr << "ERROR: No print function for var_type="
                      << static_cast<int>(vt) << "\n";
            continue;
        }

        // 4. Call it
        builder_->CreateCall(print_fn, {val});
    }

    // print() returns nothing — return a dummy i64 0
    return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
}

    /*
    llvm::Value* LLVMCodeGen::codegen_initializer(InitializerCall* expr) {
    // Find struct or class declaration
    StructDecl* struct_decl = find_struct(expr->type_name);
    ClassDecl* class_decl = find_class(expr->type_name);

    if (!struct_decl && !class_decl) {
        error("Unknown type: " + expr->type_name);
    }

    // Get field declarations
    std::vector<FieldDecl>& fields = struct_decl ?
        struct_decl->fields : class_decl->fields;

    // Build argument list with defaults
    std::vector<llvm::Value*> args;

    for (auto& field : fields) {
        llvm::Value* arg = nullptr;

        // Check if argument provided
        for (auto& named_arg : expr->args) {
            if (named_arg.name == field.name) {
                arg = codegen_expr(named_arg.value.get());
                break;
            }
        }

        // If not provided, use default
        if (!arg) {
            if (field.default_value) {
                arg = codegen_expr(field.default_value.get());
            } else if (is_option_type(field.resolved_type)) {
                // Optional fields default to None
                llvm::Function* none_fn = module_->getFunction("runtime_option_none");
                arg = builder_->CreateCall(none_fn);
            } else {
                error("No value for required field: " + field.name);
            }
        }

        args.push_back(arg);
    }

    // Call initializer
    llvm::Function* init_fn = module_->getFunction("init_" + expr->type_name);
    return builder_->CreateCall(init_fn, args);
}
Step 5: Method Codegen
cppvoid LLVMCodeGen::codegen_class_method(ClassDecl* class_decl,
                                        FunctionDecl* method) {
    // Mangle name: ClassName_methodName
    std::string mangled = class_decl->name + "_" + method->name;

    // First param is always self
    std::vector<llvm::Type*> param_types;
    param_types.push_back(llvm::PointerType::getUnqual(*context_));  // self

    for (auto& param : method->parameters) {
        param_types.push_back(get_llvm_type(param.type));
    }

    llvm::Type* return_type = method->return_type ?
        get_llvm_type(*method->return_type) :
        llvm::Type::getVoidTy(*context_);

    llvm::FunctionType* func_type = llvm::FunctionType::get(
        return_type, param_types, false
    );

    llvm::Function* func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        mangled,
        module_.get()
    );

    // Name parameters
    auto arg_it = func->args().begin();
    arg_it->setName("self");
    ++arg_it;

    for (auto& param : method->parameters) {
        arg_it->setName(param.name);
        ++arg_it;
    }

    // Generate body
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", func);
    builder_->SetInsertPoint(entry);

    // Store self in current context
    current_self_ = &*(func->args().begin());
    current_class_ = class_decl;

    // Generate method body
    codegen_block(method->body.get());

    // Clean up
    current_self_ = nullptr;
    current_class_ = nullptr;
}

// Method call: obj.method(args)
llvm::Value* LLVMCodeGen::codegen_method_call(MethodCallExpr* expr) {
    // Get object
    llvm::Value* obj = codegen_expr(expr->object.get());

    // Get class type
    std::string class_name = get_type(expr->object.get()).name;

    // Mangle method name
    std::string mangled = class_name + "_" + expr->method_name;

    // Get function
    llvm::Function* method = module_->getFunction(mangled);

    // Build args: [self, arg1, arg2, ...]
    std::vector<llvm::Value*> args;
    args.push_back(obj);  // self

    for (auto& arg : expr->arguments) {
        args.push_back(codegen_expr(arg.get()));
    }

    return builder_->CreateCall(method, args);
}

// Field access: self.field_name
llvm::Value* LLVMCodeGen::codegen_self_field_access(std::string field_name) {
    if (!current_self_) {
        error("'self' used outside method context");
    }

    size_t field_idx = get_field_index(current_class_->name, field_name);

    llvm::Function* get_fn = module_->getFunction("runtime_class_get_field_int");
    llvm::Value* idx = llvm::ConstantInt::get(*context_, llvm::APInt(64, field_idx));

    return builder_->CreateCall(get_fn, {current_self_, idx});
}

// Field assignment: self.field_name = value
void LLVMCodeGen::codegen_self_field_assign(std::string field_name,
                                             llvm::Value* value) {
    if (!current_self_) {
        error("'self' used outside method context");
    }

    size_t field_idx = get_field_index(current_class_->name, field_name);

    llvm::Function* set_fn = module_->getFunction("runtime_class_set_field_int");
    llvm::Value* idx = llvm::ConstantInt::get(*context_, llvm::APInt(64, field_idx));

    builder_->CreateCall(set_fn, {current_self_, idx, value});
}

*/



// -----------------------------------------19-FEB---------------------

// ============================================================================
// STEP 1: Generate memberwise initializer during struct/class declaration
// ============================================================================

void LLVMCodeGen::codegen_struct_decl(aithon::parser::ast::StructDecl* decl) {
    using namespace aithon::parser::ast;

    StructInfo info;
    std::vector<llvm::Type*> llvm_field_types;

    // Process fields
    for (size_t i = 0; i < decl->fields.size(); ++i) {
        auto& field = decl->fields[i];
        info.field_names.push_back(field.name);
        info.field_indices[field.name] = i;

        // Infer field type
        VarType field_type = VarType::UNKNOWN;
        if (field.default_value) {
            field_type = infer_var_type(field.default_value.get());
        } else if (field.type_annotation) {
            field_type = parse_type_annotation(*field.type_annotation);
        }
        info.field_types.push_back(field_type);

        // Map to LLVM type
        llvm::Type* llvm_type = nullptr;
        switch (field_type) {
            case VarType::INT:
                llvm_type = llvm::Type::getInt64Ty(*context_);
                break;
            case VarType::FLOAT:
                llvm_type = llvm::Type::getDoubleTy(*context_);
                break;
            case VarType::BOOL:
                llvm_type = llvm::Type::getInt1Ty(*context_);
                break;
            case VarType::STRING:
            case VarType::LIST:
            case VarType::DICT:
                llvm_type = llvm::PointerType::getUnqual(*context_);
                break;
            default:
                llvm_type = llvm::Type::getInt64Ty(*context_);
                break;
        }
        llvm_field_types.push_back(llvm_type);
    }

    // Create LLVM struct type
    info.llvm_type = llvm::StructType::create(*context_, llvm_field_types, decl->name);
    struct_types_[decl->name] = info;

    // ✅ GENERATE MEMBERWISE INITIALIZER
    generate_struct_memberwise_init(decl, info);

    std::cerr << "Defined struct: " << decl->name
              << " with " << decl->fields.size() << " fields\n";

    // ✅ ADD THIS CHECK
    std::cerr << "After generating init, checking functions_ map:\n";
    auto it = functions_.find(decl->name);
    if (it != functions_.end()) {
        std::cerr << "  ✓ '" << decl->name << "' IS in functions_ map\n";
    } else {
        std::cerr << "  ✗ '" << decl->name << "' NOT in functions_ map!\n";
    }
}

// ============================================================================
// STEP 2: Generate the memberwise initializer function
// ============================================================================

void LLVMCodeGen::generate_struct_memberwise_init(
    aithon::parser::ast::StructDecl* decl,
    StructInfo& info)
{
    // Function name: struct_name (e.g., "Point")
    // This way Point(...) is just a function call!
    std::string func_name = decl->name;

    // Build parameter types (all fields)
    std::vector<llvm::Type*> param_types;
    for (size_t i = 0; i < info.field_names.size(); ++i) {
        param_types.push_back(info.llvm_type->getElementType(i));
    }

    // Return type: the struct itself (by value)
    llvm::FunctionType* func_type = llvm::FunctionType::get(
        info.llvm_type,  // Returns struct by value
        param_types,
        false
    );

    // Create the function
    llvm::Function* init_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        func_name,
        module_.get()
    );

    // Store in functions map so it can be called
    functions_[func_name] = init_func;

    // Generate function body
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", init_func);

    // Save current state
    llvm::BasicBlock* saved_block = builder_->GetInsertBlock();
    llvm::Function* saved_func = current_function_;

    builder_->SetInsertPoint(entry);
    current_function_ = init_func;

    // Allocate struct on stack
    llvm::AllocaInst* struct_ptr = builder_->CreateAlloca(
        info.llvm_type, nullptr, "struct_tmp"
    );

    // Set each field from parameters
    size_t idx = 0;
    for (auto& arg : init_func->args()) {
        arg.setName(info.field_names[idx]);

        llvm::Value* field_ptr = builder_->CreateStructGEP(
            info.llvm_type, struct_ptr, idx
        );
        builder_->CreateStore(&arg, field_ptr);
        idx++;
    }

    // Load and return struct by value
    llvm::Value* result = builder_->CreateLoad(
        info.llvm_type, struct_ptr, "result"
    );
    builder_->CreateRet(result);

    // Restore state
    if (saved_block) {
        builder_->SetInsertPoint(saved_block);
    }
    current_function_ = saved_func;

    std::cerr << "Generated memberwise initializer: " << func_name << "\n";

    // ✅ ADD THIS CHECK
    std::cerr << "Generated memberwise initializer: " << func_name << "\n";
    std::cerr << "  Stored in functions_['" << func_name << "']\n";

    auto it = functions_.find(func_name);
    if (it != functions_.end()) {
        std::cerr << "  ✓ Verified: Found in functions_ map\n";
    } else {
        std::cerr << "  ✗ ERROR: NOT found in functions_ map!\n";
    }
}

// ============================================================================
// STEP 3: Generate memberwise initializer for classes (heap-allocated)
// ============================================================================

void LLVMCodeGen::codegen_class_decl(aithon::parser::ast::ClassDecl* decl) {
    using namespace aithon::parser::ast;

    ClassInfo info;
    info.name = decl->name;

    // Process fields
    for (size_t i = 0; i < decl->fields.size(); ++i) {
        auto& field = decl->fields[i];
        info.field_names.push_back(field.name);
        info.field_indices[field.name] = i;

        VarType field_type = VarType::UNKNOWN;
        if (field.default_value) {
            field_type = infer_var_type(field.default_value.get());
        } else if (field.type_annotation) {
            field_type = parse_type_annotation(*field.type_annotation);
        }
        info.field_types.push_back(field_type);
    }

    class_types_[decl->name] = info;

    // ✅ GENERATE MEMBERWISE INITIALIZER
    generate_class_memberwise_init(decl, info);

    // Process methods (same as before)
    for (auto& method : decl->methods) {
        std::string mangled_name = decl->name + "_" + method->name;

        std::vector<llvm::Type*> param_types;
        param_types.push_back(llvm::PointerType::getUnqual(*context_));  // self

        for (size_t i = 0; i < method->parameters.size(); ++i) {
            param_types.push_back(llvm::Type::getInt64Ty(*context_));
        }

        llvm::FunctionType* func_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            param_types,
            false
        );

        llvm::Function* llvm_func = llvm::Function::Create(
            func_type,
            llvm::Function::ExternalLinkage,
            mangled_name,
            module_.get()
        );

        functions_[mangled_name] = llvm_func;
        info.methods.push_back(llvm_func);

        // Generate method body (save for later to avoid conflicts)
        // ... (method body generation code) ...
    }

    std::cerr << "Defined class: " << decl->name
              << " with " << decl->fields.size() << " fields and "
              << decl->methods.size() << " methods\n";
}

void LLVMCodeGen::generate_class_memberwise_init(
    aithon::parser::ast::ClassDecl* decl,
    ClassInfo& info)
{
    std::string func_name = decl->name;

    // Build parameter types
    std::vector<llvm::Type*> param_types;
    for (size_t i = 0; i < info.field_names.size(); ++i) {
        switch (info.field_types[i]) {
            case VarType::INT:
                param_types.push_back(llvm::Type::getInt64Ty(*context_));
                break;
            case VarType::FLOAT:
                param_types.push_back(llvm::Type::getDoubleTy(*context_));
                break;
            case VarType::BOOL:
                param_types.push_back(llvm::Type::getInt1Ty(*context_));
                break;
            default:
                param_types.push_back(llvm::PointerType::getUnqual(*context_));
                break;
        }
    }

    // Return type: HeapObject* (class instance)
    llvm::FunctionType* func_type = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context_),
        param_types,
        false
    );

    llvm::Function* init_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        func_name,
        module_.get()
    );

    functions_[func_name] = init_func;

    // Generate function body
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", init_func);

    llvm::BasicBlock* saved_block = builder_->GetInsertBlock();
    llvm::Function* saved_func = current_function_;

    builder_->SetInsertPoint(entry);
    current_function_ = init_func;

    // Create class instance: runtime_class_create(name, num_fields)
    llvm::Function* create_fn = module_->getFunction("runtime_class_create");
    if (!create_fn) {
        std::cerr << "ERROR: runtime_class_create not declared\n";
        return;
    }

    llvm::Value* class_name = builder_->CreateGlobalStringPtr(decl->name);
    llvm::Value* num_fields = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(*context_), info.field_names.size()
    );

    llvm::Value* obj_ptr = builder_->CreateCall(create_fn, {class_name, num_fields});

    // Set each field from parameters
    size_t idx = 0;
    for (auto& arg : init_func->args()) {
        arg.setName(info.field_names[idx]);

        llvm::Value* field_idx_val = llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(*context_), idx
        );

        // Call appropriate setter
        llvm::Function* set_fn = nullptr;
        switch (info.field_types[idx]) {
            case VarType::INT:
                set_fn = module_->getFunction("runtime_class_set_field_int");
                break;
            case VarType::FLOAT:
                set_fn = module_->getFunction("runtime_class_set_field_float");
                break;
            case VarType::BOOL:
                set_fn = module_->getFunction("runtime_class_set_field_bool");
                break;
            default:
                set_fn = module_->getFunction("runtime_class_set_field_ptr");
                break;
        }

        if (set_fn) {
            builder_->CreateCall(set_fn, {obj_ptr, field_idx_val, &arg});
        }

        idx++;
    }

    // Return the object (ref_count = 1)
    builder_->CreateRet(obj_ptr);

    if (saved_block) {
        builder_->SetInsertPoint(saved_block);
    }
    current_function_ = saved_func;

    std::cerr << "Generated memberwise initializer: " << func_name << "\n";
}

// ============================================================================
// STEP 5: codegen_struct_init — create struct instance (stack)
// ============================================================================

llvm::Value* LLVMCodeGen::codegen_struct_init(
        const std::string& struct_name,
        const std::vector<llvm::Value*>& field_values)
{
    auto it = struct_types_.find(struct_name);
    if (it == struct_types_.end()) {
        std::cerr << "ERROR: Unknown struct: " << struct_name << "\n";
        return nullptr;
    }

    StructInfo& info = it->second;

    // Allocate struct on stack
    llvm::AllocaInst* struct_ptr = builder_->CreateAlloca(
        info.llvm_type, nullptr, "struct_tmp"
    );

    // Set each field
    for (size_t i = 0; i < field_values.size() && i < info.field_names.size(); ++i) {
        llvm::Value* field_ptr = builder_->CreateStructGEP(
            info.llvm_type, struct_ptr, i, info.field_names[i] + "_ptr"
        );
        builder_->CreateStore(field_values[i], field_ptr);
    }

    // Load the entire struct value (for copy semantics)
    return builder_->CreateLoad(info.llvm_type, struct_ptr, struct_name + "_val");
}

// ============================================================================
// STEP 6: codegen_class_init — create class instance (heap, ref_count=1)
// ============================================================================

llvm::Value* LLVMCodeGen::codegen_class_init(
    const std::string& class_name,
    const std::vector<llvm::Value*>& field_values)
{
    auto it = class_types_.find(class_name);
    if (it == class_types_.end()) {
        std::cerr << "ERROR: Unknown class: " << class_name << "\n";
        return nullptr;
    }

    ClassInfo& info = it->second;

    // runtime_class_create(name, num_fields) → ptr with ref_count=1
    llvm::Function* create_fn = module_->getFunction("runtime_class_create");
    if (!create_fn) {
        std::cerr << "ERROR: runtime_class_create not declared\n";
        return nullptr;
    }

    llvm::Value* name_str = builder_->CreateGlobalString(class_name);
    llvm::Value* num_fields = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(*context_), info.field_names.size()
    );

    llvm::Value* obj_ptr = builder_->CreateCall(create_fn, {name_str, num_fields});

    // Set each field using runtime_class_set_field_*
    for (size_t i = 0; i < field_values.size() && i < info.field_names.size(); ++i) {
        llvm::Value* field_idx = llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(*context_), i
        );

        llvm::Function* set_fn = nullptr;
        switch (info.field_types[i]) {
            case VarType::INT:
                set_fn = module_->getFunction("runtime_class_set_field_int");
                break;
            case VarType::FLOAT:
                set_fn = module_->getFunction("runtime_class_set_field_float");
                break;
            case VarType::BOOL:
                set_fn = module_->getFunction("runtime_class_set_field_bool");
                break;
            case VarType::STRING:
            case VarType::LIST:
            case VarType::DICT:
                set_fn = module_->getFunction("runtime_class_set_field_ptr");
                break;
            default:
                set_fn = module_->getFunction("runtime_class_set_field_int");
                break;
        }

        if (set_fn) {
            builder_->CreateCall(set_fn, {obj_ptr, field_idx, field_values[i]});
        }
    }

    return obj_ptr;  // ref_count=1
}

// ============================================================================
// STEP 7: codegen_struct_field_access — get field from struct value
// ============================================================================

llvm::Value* LLVMCodeGen::codegen_struct_field_access(
    llvm::Value* struct_val,
    const std::string& struct_name,
    const std::string& field_name)
{
    auto it = struct_types_.find(struct_name);
    if (it == struct_types_.end()) {
        std::cerr << "ERROR: Unknown struct: " << struct_name << "\n";
        return nullptr;
    }

    StructInfo& info = it->second;
    auto field_it = info.field_indices.find(field_name);
    if (field_it == info.field_indices.end()) {
        std::cerr << "ERROR: Struct " << struct_name << " has no field: " << field_name << "\n";
        return nullptr;
    }

    size_t field_idx = field_it->second;

    // Extract field from struct value
    return builder_->CreateExtractValue(struct_val, {static_cast<unsigned>(field_idx)}, field_name);
}

// ============================================================================
// STEP 8: codegen_class_field_access — get field from class object
// ============================================================================

llvm::Value* LLVMCodeGen::codegen_class_field_access(
    llvm::Value* obj_ptr,
    const std::string& class_name,
    const std::string& field_name)
{
    auto it = class_types_.find(class_name);
    if (it == class_types_.end()) {
        std::cerr << "ERROR: Unknown class: " << class_name << "\n";
        return nullptr;
    }

    ClassInfo& info = it->second;
    auto field_it = info.field_indices.find(field_name);
    if (field_it == info.field_indices.end()) {
        std::cerr << "ERROR: Class " << class_name << " has no field: " << field_name << "\n";
        return nullptr;
    }

    size_t field_idx = field_it->second;
    llvm::Value* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), field_idx);

    // runtime_class_get_field_*(obj, field_idx)
    llvm::Function* get_fn = nullptr;
    switch (info.field_types[field_idx]) {
        case VarType::INT:
            get_fn = module_->getFunction("runtime_class_get_field_int");
            break;
        case VarType::FLOAT:
            get_fn = module_->getFunction("runtime_class_get_field_float");
            break;
        case VarType::BOOL:
            get_fn = module_->getFunction("runtime_class_get_field_bool");
            break;
        case VarType::STRING:
        case VarType::LIST:
        case VarType::DICT:
            get_fn = module_->getFunction("runtime_class_get_field_ptr");
            break;
        default:
            get_fn = module_->getFunction("runtime_class_get_field_int");
            break;
    }

    if (!get_fn) {
        std::cerr << "ERROR: runtime_class_get_field not found\n";
        return nullptr;
    }

    return builder_->CreateCall(get_fn, {obj_ptr, idx}, field_name);
}

// ============================================================================
// STEP 9: codegen_struct_field_assign — update field in struct (via pointer)
// ============================================================================

void LLVMCodeGen::codegen_struct_field_assign(
    llvm::Value* struct_ptr,
    const std::string& struct_name,
    const std::string& field_name,
    llvm::Value* value)
{
    auto it = struct_types_.find(struct_name);
    if (it == struct_types_.end()) {
        std::cerr << "ERROR: Unknown struct: " << struct_name << "\n";
        return;
    }

    StructInfo& info = it->second;
    auto field_it = info.field_indices.find(field_name);
    if (field_it == info.field_indices.end()) {
        std::cerr << "ERROR: Struct " << struct_name << " has no field: " << field_name << "\n";
        return;
    }

    size_t field_idx = field_it->second;

    // Get pointer to field
    llvm::Value* field_ptr = builder_->CreateStructGEP(
        info.llvm_type, struct_ptr, field_idx, field_name + "_ptr"
    );

    // Store new value
    builder_->CreateStore(value, field_ptr);
}

// ============================================================================
// STEP 10: codegen_class_field_assign — update field in class object
// ============================================================================

void LLVMCodeGen::codegen_class_field_assign(
    llvm::Value* obj_ptr,
    const std::string& class_name,
    const std::string& field_name,
    llvm::Value* value)
{
    auto it = class_types_.find(class_name);
    if (it == class_types_.end()) {
        std::cerr << "ERROR: Unknown class: " << class_name << "\n";
        return;
    }

    ClassInfo& info = it->second;
    auto field_it = info.field_indices.find(field_name);
    if (field_it == info.field_indices.end()) {
        std::cerr << "ERROR: Class " << class_name << " has no field: " << field_name << "\n";
        return;
    }

    size_t field_idx = field_it->second;
    llvm::Value* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), field_idx);

    // runtime_class_set_field_*(obj, field_idx, value)
    llvm::Function* set_fn = nullptr;
    switch (info.field_types[field_idx]) {
        case VarType::INT:
            set_fn = module_->getFunction("runtime_class_set_field_int");
            break;
        case VarType::FLOAT:
            set_fn = module_->getFunction("runtime_class_set_field_float");
            break;
        case VarType::BOOL:
            set_fn = module_->getFunction("runtime_class_set_field_bool");
            break;
        case VarType::STRING:
        case VarType::LIST:
        case VarType::DICT:
            set_fn = module_->getFunction("runtime_class_set_field_ptr");
            break;
        default:
            set_fn = module_->getFunction("runtime_class_set_field_int");
            break;
    }

    if (!set_fn) {
        std::cerr << "ERROR: runtime_class_set_field not found\n";
        return;
    }

    builder_->CreateCall(set_fn, {obj_ptr, idx, value});
}

// ============================================================================
// STEP 11: Helper — parse type annotation string
// ============================================================================

LLVMCodeGen::VarType LLVMCodeGen::parse_type_annotation(const std::string& type_str) {
    if (type_str == "int")    return VarType::INT;
    if (type_str == "float")  return VarType::FLOAT;
    if (type_str == "bool")   return VarType::BOOL;
    if (type_str == "str")    return VarType::STRING;
    if (type_str == "list")   return VarType::LIST;
    if (type_str == "dict")   return VarType::DICT;

    // Handle Option[T] — for now, just extract T
    if (type_str.find("Option[") == 0) {
        size_t start = type_str.find('[') + 1;
        size_t end = type_str.find(']');
        if (end != std::string::npos) {
            std::string inner = type_str.substr(start, end - start);
            return parse_type_annotation(inner);
        }
    }

    return VarType::UNKNOWN;
}



}