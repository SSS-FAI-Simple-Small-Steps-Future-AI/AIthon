#pragma once

#include "../lexer/token.h"
#include "../utils/error_reporter.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <initializer_list>

namespace aithon::parser {

class Parser {
private:
    std::vector<lexer::Token> tokens_;
    utils::ErrorReporter& error_reporter_;
    size_t current_;
    
    // Small inline helper functions (performance-critical)
    [[nodiscard]] const lexer::Token& current() const { return tokens_[current_]; }
    [[nodiscard]] const lexer::Token& previous() const { return tokens_[current_ - 1]; }
    [[nodiscard]] const lexer::Token& peek() const {
        if (current_ + 1 < tokens_.size()) return tokens_[current_ + 1];
        return tokens_.back();
    }
    [[nodiscard]] bool is_at_end() const { 
        return current().type == lexer::TokenType::END_OF_FILE;
    }
    
    // All other methods in .cpp
    const lexer::Token& advance();
    bool check(lexer::TokenType type) const;
    bool match(lexer::TokenType type);
    bool match(std::initializer_list<lexer::TokenType> types);
    void consume(lexer::TokenType type, const std::string& message);
    void skip_newlines();
    
    // Statement parsing
    std::unique_ptr<ast::Stmt> parse_statement();
    std::unique_ptr<ast::FunctionDecl> parse_function_decl();
    std::unique_ptr<ast::Block> parse_block();
    std::unique_ptr<ast::IfStmt> parse_if_stmt();
    std::unique_ptr<ast::WhileStmt> parse_while_stmt();
    std::unique_ptr<ast::ForStmt> parse_for_stmt();
    std::unique_ptr<ast::ReturnStmt> parse_return_stmt();
    std::unique_ptr<ast::Stmt> parse_assignment_or_expr();
    
    // Expression parsing
    std::unique_ptr<ast::Expr> parse_expression();
    std::unique_ptr<ast::Expr> parse_logical_or();
    std::unique_ptr<ast::Expr> parse_logical_and();
    std::unique_ptr<ast::Expr> parse_equality();
    std::unique_ptr<ast::Expr> parse_comparison();
    std::unique_ptr<ast::Expr> parse_term();
    std::unique_ptr<ast::Expr> parse_factor();
    std::unique_ptr<ast::Expr> parse_unary();
    std::unique_ptr<ast::Expr> parse_power();
    std::unique_ptr<ast::Expr> parse_postfix();
    std::unique_ptr<ast::Expr> parse_primary();
    std::unique_ptr<ast::Expr> parse_list_literal();
    std::unique_ptr<ast::Expr> parse_dict_literal();
    std::unique_ptr<ast::StructDecl> parse_struct_decl();
    std::unique_ptr<ast::ClassDecl> parse_class_decl();


    
public:
    Parser(std::vector<lexer::Token> tokens, utils::ErrorReporter& reporter);
    std::unique_ptr<ast::Module> parse();
};

}