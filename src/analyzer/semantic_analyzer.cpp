#include "../../include/analyzer/semantic_analyzer.h"

namespace aithon::analyzer {

// ============================================================================
// DataType Helper
// ============================================================================

const char* type_to_string(DataType type) {
    switch (type) {
        case DataType::INTEGER: return "int";
        case DataType::FLOAT: return "float";
        case DataType::STRING: return "str";
        case DataType::BOOL: return "bool";
        case DataType::NONE: return "None";
        case DataType::LIST: return "list";
        case DataType::DICT: return "dict";
        case DataType::FUNCTION: return "function";
        case DataType::UNKNOWN: return "unknown";
    }
    return "unknown";
}

// ============================================================================
// Symbol
// ============================================================================

Symbol::Symbol(std::string n, DataType t, int line, bool is_func)
    : name(std::move(n)), type(t), is_initialized(false), 
      is_function(is_func), declaration_line(line) {}

// ============================================================================
// SymbolTable
// ============================================================================

SymbolTable::SymbolTable() {
    scopes_.emplace_back();
}

void SymbolTable::push_scope() {
    scopes_.emplace_back();
}

void SymbolTable::pop_scope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
    }
}

bool SymbolTable::declare(const Symbol& symbol) {
    auto& current_scope = scopes_.back();
    
    if (current_scope.find(symbol.name) != current_scope.end()) {
        return false;
    }
    
    current_scope.insert({symbol.name, symbol});
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

bool SymbolTable::update_type(const std::string& name, DataType new_type) {
    Symbol* symbol = lookup(name);
    if (!symbol) return false;
    
    symbol->type = new_type;
    symbol->is_initialized = true;
    return true;
}

// ============================================================================
// SemanticAnalyzer
// ============================================================================

SemanticAnalyzer::SemanticAnalyzer(utils::ErrorReporter& reporter)
    : error_reporter_(reporter), in_function_(false), in_loop_(false) {
    declare_builtins();
}

void SemanticAnalyzer::declare_builtins() {
    const std::vector<std::string> builtins = {
        "print", "len", "range", "str", "int", "float", 
        "bool", "list", "dict", "input", "type"
    };
    
    for (const auto& name : builtins) {
        Symbol builtin(name, DataType::FUNCTION, 0, true);
        builtin.is_initialized = true;
        symbol_table_.declare(builtin);
    }
}

DataType SemanticAnalyzer::infer_type(parser::ast::Expr* expr) {
    if (!expr) return DataType::UNKNOWN;
    
    if (dynamic_cast<parser::ast::IntegerLiteral*>(expr)) {
        return DataType::INTEGER;
    }
    
    if (dynamic_cast<parser::ast::FloatLiteral*>(expr)) {
        return DataType::FLOAT;
    }
    
    if (dynamic_cast<parser::ast::StringLiteral*>(expr)) {
        return DataType::STRING;
    }
    
    if (dynamic_cast<parser::ast::BoolLiteral*>(expr)) {
        return DataType::BOOL;
    }
    
    if (dynamic_cast<parser::ast::NoneLiteral*>(expr)) {
        return DataType::NONE;
    }
    
    if (dynamic_cast<parser::ast::ListExpr*>(expr)) {
        return DataType::LIST;
    }
    
    if (dynamic_cast<parser::ast::DictExpr*>(expr)) {
        return DataType::DICT;
    }
    
    if (auto* ident = dynamic_cast<parser::ast::Identifier*>(expr)) {
        Symbol* symbol = symbol_table_.lookup(ident->name);
        return symbol ? symbol->type : DataType::UNKNOWN;
    }
    
    if (auto* binop = dynamic_cast<parser::ast::BinaryOp*>(expr)) {
        DataType left_type = infer_type(binop->left.get());
        DataType right_type = infer_type(binop->right.get());
        
        if (binop->op == parser::ast::BinaryOp::Op::ADD ||
            binop->op == parser::ast::BinaryOp::Op::SUB ||
            binop->op == parser::ast::BinaryOp::Op::MUL ||
            binop->op == parser::ast::BinaryOp::Op::DIV) {
            
            if (left_type == DataType::FLOAT || right_type == DataType::FLOAT) {
                return DataType::FLOAT;
            }
            return DataType::INTEGER;
        }
        
        if (binop->op == parser::ast::BinaryOp::Op::EQUAL ||
            binop->op == parser::ast::BinaryOp::Op::NOT_EQUAL ||
            binop->op == parser::ast::BinaryOp::Op::LESS ||
            binop->op == parser::ast::BinaryOp::Op::LESS_EQUAL ||
            binop->op == parser::ast::BinaryOp::Op::GREATER ||
            binop->op == parser::ast::BinaryOp::Op::GREATER_EQUAL) {
            return DataType::BOOL;
        }
        
        if (binop->op == parser::ast::BinaryOp::Op::AND ||
            binop->op == parser::ast::BinaryOp::Op::OR) {
            return DataType::BOOL;
        }
    }
    
    return DataType::UNKNOWN;
}

bool SemanticAnalyzer::check_function_has_return(parser::ast::Block* block) {
    for (auto& stmt : block->statements) {
        if (auto* ret_stmt = dynamic_cast<parser::ast::ReturnStmt*>(stmt.get())) {
            if (ret_stmt->value) {
                return true;
            }
        }
        
        if (auto* if_stmt = dynamic_cast<parser::ast::IfStmt*>(stmt.get())) {
            if (check_function_has_return(if_stmt->then_block.get())) {
                return true;
            }
            if (if_stmt->else_block && check_function_has_return(if_stmt->else_block.get())) {
                return true;
            }
        }
        
        if (auto* while_stmt = dynamic_cast<parser::ast::WhileStmt*>(stmt.get())) {
            if (check_function_has_return(while_stmt->body.get())) {
                return true;
            }
        }
        
        if (auto* for_stmt = dynamic_cast<parser::ast::ForStmt*>(stmt.get())) {
            if (check_function_has_return(for_stmt->body.get())) {
                return true;
            }
        }
    }
    
    return false;
}

void SemanticAnalyzer::analyze_expr(parser::ast::Expr* expr) {
    if (!expr) return;
    
    if (auto* ident = dynamic_cast<parser::ast::Identifier*>(expr)) {
        Symbol* symbol = symbol_table_.lookup(ident->name);
        if (!symbol) {
            error_reporter_.syntax_error(
                lexer::SourceLocation(0, 0),
                "undefined variable '" + ident->name + "'"
            );
        }
    }
    
    if (auto* binop = dynamic_cast<parser::ast::BinaryOp*>(expr)) {
        analyze_expr(binop->left.get());
        analyze_expr(binop->right.get());
    }
    
    if (auto* unop = dynamic_cast<parser::ast::UnaryOp*>(expr)) {
        analyze_expr(unop->operand.get());
    }
    
    if (auto* call = dynamic_cast<parser::ast::CallExpr*>(expr)) {
        analyze_expr(call->callee.get());
        for (auto& arg : call->arguments) {
            analyze_expr(arg.get());
        }
        
        if (auto* callee = dynamic_cast<parser::ast::Identifier*>(call->callee.get())) {
            functions_used_as_values_.insert(callee->name);
        }
    }
    
    if (auto* list = dynamic_cast<parser::ast::ListExpr*>(expr)) {
        for (auto& elem : list->elements) {
            analyze_expr(elem.get());
        }
    }
    
    if (auto* dict = dynamic_cast<parser::ast::DictExpr*>(expr)) {
        for (auto& [key, value] : dict->pairs) {
            analyze_expr(key.get());
            analyze_expr(value.get());
        }
    }
}

void SemanticAnalyzer::analyze_stmt(parser::ast::Stmt* stmt) {
    if (!stmt) return;
    
    if (auto* expr_stmt = dynamic_cast<parser::ast::ExprStmt*>(stmt)) {
        analyze_expr(expr_stmt->expression.get());
    }
    
    else if (auto* assignment = dynamic_cast<parser::ast::Assignment*>(stmt)) {
        analyze_expr(assignment->value.get());
        
        DataType value_type = infer_type(assignment->value.get());
        Symbol* existing = symbol_table_.lookup(assignment->name);
        
        if (existing) {
            if (existing->type != DataType::UNKNOWN && 
                value_type != DataType::UNKNOWN &&
                existing->type != value_type) {
                
                error_reporter_.syntax_error(
                    lexer::SourceLocation(0, 0),
                    "type mismatch: cannot assign " + 
                    std::string(type_to_string(value_type)) + 
                    " to variable '" + assignment->name + 
                    "' of type " + std::string(type_to_string(existing->type))
                );
                return;
            }
            
            existing->is_initialized = true;
        } else {
            Symbol new_var(assignment->name, value_type, 0);
            new_var.is_initialized = true;
            symbol_table_.declare(new_var);
        }
    }
    
    else if (auto* block = dynamic_cast<parser::ast::Block*>(stmt)) {
        symbol_table_.push_scope();
        for (auto& s : block->statements) {
            analyze_stmt(s.get());
        }
        symbol_table_.pop_scope();
    }
    
    else if (auto* if_stmt = dynamic_cast<parser::ast::IfStmt*>(stmt)) {
        analyze_expr(if_stmt->condition.get());
        analyze_stmt(if_stmt->then_block.get());
        if (if_stmt->else_block) {
            analyze_stmt(if_stmt->else_block.get());
        }
    }
    
    else if (auto* while_stmt = dynamic_cast<parser::ast::WhileStmt*>(stmt)) {
        bool prev_in_loop = in_loop_;
        in_loop_ = true;
        analyze_expr(while_stmt->condition.get());
        analyze_stmt(while_stmt->body.get());
        in_loop_ = prev_in_loop;
    }
    
    else if (auto* for_stmt = dynamic_cast<parser::ast::ForStmt*>(stmt)) {
        bool prev_in_loop = in_loop_;
        in_loop_ = true;
        
        symbol_table_.push_scope();
        
        Symbol loop_var(for_stmt->variable, DataType::UNKNOWN, 0);
        loop_var.is_initialized = true;
        symbol_table_.declare(loop_var);
        
        analyze_expr(for_stmt->iterable.get());
        analyze_stmt(for_stmt->body.get());
        
        symbol_table_.pop_scope();
        in_loop_ = prev_in_loop;
    }
    
    else if (auto* ret_stmt = dynamic_cast<parser::ast::ReturnStmt*>(stmt)) {
        if (!in_function_) {
            error_reporter_.syntax_error(
                lexer::SourceLocation(0, 0),
                "'return' outside function"
            );
            return;
        }
        
        if (ret_stmt->value) {
            analyze_expr(ret_stmt->value.get());
        }
    }
    
    else if (dynamic_cast<parser::ast::BreakStmt*>(stmt)) {
        if (!in_loop_) {
            error_reporter_.syntax_error(
                lexer::SourceLocation(0, 0),
                "'break' outside loop"
            );
        }
    }
    
    else if (dynamic_cast<parser::ast::ContinueStmt*>(stmt)) {
        if (!in_loop_) {
            error_reporter_.syntax_error(
                lexer::SourceLocation(0, 0),
                "'continue' outside loop"
            );
        }
    }
    
    else if (auto* func = dynamic_cast<parser::ast::FunctionDecl*>(stmt)) {
        Symbol func_symbol(func->name, DataType::FUNCTION, 0, true);
        func_symbol.is_initialized = true;
        
        if (!symbol_table_.declare(func_symbol)) {
            error_reporter_.syntax_error(
                lexer::SourceLocation(0, 0),
                "function '" + func->name + "' already declared"
            );
            return;
        }
        
        symbol_table_.push_scope();
        bool prev_in_function = in_function_;
        in_function_ = true;
        
        for (auto& param : func->parameters) {
            Symbol param_symbol(param.name, DataType::UNKNOWN, 0);
            param_symbol.is_initialized = true;
            symbol_table_.declare(param_symbol);
        }
        
        analyze_stmt(func->body.get());
        
        bool has_return = check_function_has_return(func->body.get());
        function_has_return_[func->name] = has_return;
        
        symbol_table_.pop_scope();
        in_function_ = prev_in_function;
    }
    
    else if (auto* cls = dynamic_cast<parser::ast::ClassDecl*>(stmt)) {
        Symbol class_symbol(cls->name, DataType::UNKNOWN, 0);
        class_symbol.is_initialized = true;
        symbol_table_.declare(class_symbol);

        // Analyze methods
        symbol_table_.push_scope();
        for (auto& method : cls->methods) {
            if (method) {
                analyze_stmt(method.get());
            }
        }
        symbol_table_.pop_scope();
    }
}

bool SemanticAnalyzer::analyze(parser::ast::Module* module) {
    if (!module) return false;
    
    for (auto& stmt : module->statements) {
        analyze_stmt(stmt.get());
    }
    
    for (const auto& func_name : functions_used_as_values_) {
        auto it = function_has_return_.find(func_name);
        
        Symbol* symbol = symbol_table_.lookup(func_name);
        if (symbol && symbol->declaration_line == 0) {
            continue;
        }
        
        if (it != function_has_return_.end() && !it->second) {
            error_reporter_.syntax_error(
                lexer::SourceLocation(0, 0),
                "function '" + func_name + 
                "' is used in an assignment but does not return a value"
            );
        }
    }
    
    return !error_reporter_.has_errors();
}

}
