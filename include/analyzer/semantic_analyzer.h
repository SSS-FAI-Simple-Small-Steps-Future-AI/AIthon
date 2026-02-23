#pragma once

#include "../parser/ast.h"
#include "../utils/error_reporter.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <memory>

namespace aithon::analyzer {

    enum class DataType {
        INTEGER,
        FLOAT,
        STRING,
        BOOL,
        NONE,
        LIST,
        DICT,
        FUNCTION,
        UNKNOWN
    };

    const char* type_to_string(DataType type);

    struct Symbol {
        std::string name;
        DataType type;
        bool is_initialized;
        bool is_function;
        int declaration_line;

        Symbol(std::string n, DataType t, int line, bool is_func = false);
    };

    class SymbolTable {
    private:
        std::vector<std::unordered_map<std::string, Symbol>> scopes_;

    public:
        SymbolTable();

        void push_scope();
        void pop_scope();
        bool declare(const Symbol& symbol);
        Symbol* lookup(const std::string& name);
        bool update_type(const std::string& name, DataType new_type);
    };

    class SemanticAnalyzer {
    private:
        utils::ErrorReporter& error_reporter_;
        SymbolTable symbol_table_;
        bool in_function_;
        bool in_loop_;

        std::unordered_set<std::string> functions_used_as_values_;
        std::unordered_map<std::string, bool> function_has_return_;

        void declare_builtins();
        DataType infer_type(parser::ast::Expr* expr);
        bool check_function_has_return(parser::ast::Block* block);
        void analyze_expr(parser::ast::Expr* expr);
        void analyze_stmt(parser::ast::Stmt* stmt);

    public:
        explicit SemanticAnalyzer(utils::ErrorReporter& reporter);
        bool analyze(parser::ast::Module* module);
    };

}