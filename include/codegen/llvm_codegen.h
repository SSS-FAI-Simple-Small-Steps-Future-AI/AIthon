#pragma once


#include "../parser/ast.h"
#include "../utils/error_reporter.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>

namespace aithon::codegen {

class LLVMCodeGen {
private:
    // LLVM Core components
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::legacy::FunctionPassManager> fpm_;

    utils::ErrorReporter& error_reporter_;

    // Symbol tables
    // std::unordered_map<std::string, llvm::Value*> named_values_;

    enum class VarType {
        INT,
        FLOAT,
        BOOL,
        STRING,
        LIST,
        DICT,
        UNKNOWN
    };

    struct VarInfo {
        llvm::AllocaInst* alloca   = nullptr;  // default values avoid aggregate init bug
        llvm::Type*       type     = nullptr;
        VarType           var_type = VarType::UNKNOWN;
        std::string       type_name;  // ← ADD THIS (e.g., "Point", "Circle")
    };

    std::unordered_map<std::string, VarInfo> variables_;
    std::unordered_map<std::string, VarType>       var_types_;     // ← type map
    std::unordered_map<std::string, llvm::Type*>   var_llvm_types_;// ← llvm type map

    std::unordered_map<std::string, llvm::Value* >   named_values_;// ← llvm value map


    VarType         infer_var_type(parser::ast::Expr* expr);
    // llvm::Function* get_print_fn(VarType vt, llvm::Value* val);
    llvm::Value*    codegen_print_call(parser::ast::CallExpr* expr);
    llvm::Value*    codegen_call(aithon::parser::ast::CallExpr* expr);


    // Add these two method declarations:
    // VarType        infer_var_type(parser::ast::Expr* expr);
    llvm::Function* get_print_function(VarType var_type, llvm::Value* value);

    // Helper declarations
    llvm::Function*  get_print_fn_for(VarType vt, llvm::Value* val);

    std::unordered_map<std::string, llvm::Function*> functions_;

    // Current function context
    llvm::Function* current_function_;

    // Helper methods - declarations only
    void initialize_module(const std::string& module_name);
    void setup_optimization_passes();
    void declare_runtime_functions();

    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* func,
                                                 const std::string& var_name,
                                                 llvm::Type* type = nullptr);

    // Code generation for statements
    void codegen_stmt(parser::ast::Stmt* stmt);
    void codegen_function(parser::ast::FunctionDecl* func);


    void codegen_block(parser::ast::Block* block);
    void codegen_if(parser::ast::IfStmt* stmt);
    void codegen_while(parser::ast::WhileStmt* stmt);
    void codegen_for(parser::ast::ForStmt* stmt);
    void codegen_return(parser::ast::ReturnStmt* stmt);

    llvm::Value* codegen_identifier(parser::ast::Identifier* expr);
    void codegen_assignment(parser::ast::Assignment* stmt);
    void codegen_expr_stmt(parser::ast::ExprStmt* stmt);

    // Code generation for expressions
    llvm::Value* codegen_expr(parser::ast::Expr* expr);
    llvm::Value* codegen_integer(const parser::ast::IntegerLiteral* expr) const;
    llvm::Value* codegen_float(const parser::ast::FloatLiteral* expr) const;
    llvm::Value* codegen_string(const parser::ast::StringLiteral* expr) const;
    llvm::Value* codegen_bool(const parser::ast::BoolLiteral* expr) const;
    llvm::Value* codegen_none(parser::ast::NoneLiteral* expr) const;
    llvm::Value* codegen_identifier(const parser::ast::Identifier* expr);
    llvm::Value* codegen_binary_op(const parser::ast::BinaryOp* expr);
    llvm::Value* codegen_unary_op(const parser::ast::UnaryOp* expr);
    // llvm::Value* codegen_call(parser::ast::CallExpr *expr);

    llvm::Value* codegen_list(parser::ast::ListExpr* expr);
    llvm::Value* codegen_dict(parser::ast::DictExpr* expr);
    llvm::Value* codegen_index(parser::ast::IndexExpr* expr);

    // Special function generation
    [[nodiscard]] llvm::Function* generate_main_wrapper() const;

    // Structure and Class
    struct StructInfo {
        llvm::StructType*              llvm_type;
        std::vector<std::string>       field_names;
        std::vector<VarType>           field_types;
        std::map<std::string, size_t>  field_indices;
    };

    struct ClassInfo {
        std::string                    name;
        std::vector<std::string>       field_names;
        std::vector<VarType>           field_types;
        std::map<std::string, size_t>  field_indices;
        std::vector<llvm::Function*>   methods;
    };

    std::unordered_map<std::string, StructInfo> struct_types_;
    std::unordered_map<std::string, ClassInfo>  class_types_;
    std::string current_class_name_;  // for 'self' resolution

    // Methods
    void codegen_struct_decl(parser::ast::StructDecl*);
    void codegen_class_decl(parser::ast::ClassDecl*);
    VarType parse_type_annotation(const std::string&);

    llvm::Value* codegen_struct_init(const std::string& struct_name,
                                     const std::vector<llvm::Value*>& field_values);
    llvm::Value* codegen_class_init(const std::string& class_name,
                                    const std::vector<llvm::Value*>& field_values);

    llvm::Value* codegen_struct_field_access(llvm::Value* struct_val,
                                             const std::string& struct_name,
                                             const std::string& field_name);
    llvm::Value* codegen_class_field_access(llvm::Value* obj_ptr,
                                            const std::string& class_name,
                                            const std::string& field_name);

    void codegen_struct_field_assign(llvm::Value* struct_ptr,
                                     const std::string& struct_name,
                                     const std::string& field_name,
                                     llvm::Value* value);
    void codegen_class_field_assign(llvm::Value* obj_ptr,
                                    const std::string& class_name,
                                    const std::string& field_name,
                                    llvm::Value* value);


    void codegen_field_assignment(parser::ast::FieldAssignment* stmt);
    void codegen_index_assignment(parser::ast::IndexAssignment* stmt);

    llvm::Value* codegen_initializer(parser::ast::InitializerExpr* expr);
    llvm::Value* codegen_member_access(parser::ast::MemberExpr* expr);

    void generate_struct_memberwise_init(aithon::parser::ast::StructDecl* decl,
                                         StructInfo& info);

    void generate_class_memberwise_init(aithon::parser::ast::ClassDecl* decl,
                                        ClassInfo& info);

public:
    explicit LLVMCodeGen(utils::ErrorReporter& reporter);
    ~LLVMCodeGen();

    // Main code generation
    bool generate(parser::ast::Module* module, const std::string& module_name = "main_module");

    // Output methods
    [[nodiscard]] bool write_ir_to_file(const std::string& filename) const;
    [[nodiscard]] bool write_object_to_file(const std::string& filename) const;
    void dump_ir() const;

    // Optimization
    void optimize() const;

};

}