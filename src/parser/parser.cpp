#include "../../include/parser/parser.h"
#include <stdexcept>

namespace aithon::parser {

Parser::Parser(std::vector<lexer::Token> tokens, utils::ErrorReporter& reporter)
    : tokens_(std::move(tokens)), error_reporter_(reporter), current_(0) {}

// ============================================================================
// Helper Methods
// ============================================================================

const lexer::Token& Parser::advance() {
    if (!is_at_end()) current_++;
    return previous();
}

bool Parser::check(lexer::TokenType type) const {
    if (is_at_end()) return false;
    return current().type == type;
}

bool Parser::match(lexer::TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<lexer::TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

void Parser::consume(lexer::TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return;
    }
    
    error_reporter_.syntax_error_expected(
        current().location,
        token_type_to_string(type),
        "'" + current().lexeme + "'"
    );
    throw std::runtime_error("Parse error");
}

void Parser::skip_newlines() {
    while (match(lexer::TokenType::NEWLINE)) {}
}

// ============================================================================
// Statement Parsing
// ============================================================================

std::unique_ptr<ast::Stmt> Parser::parse_statement() {
    skip_newlines();
    
    if (match(lexer::TokenType::FUNC)) return parse_function_decl();
    if (match(lexer::TokenType::STRUCT)) return parse_struct_decl();
    if (match(lexer::TokenType::CLASS)) return parse_class_decl();
    if (match(lexer::TokenType::IF)) return parse_if_stmt();
    if (match(lexer::TokenType::WHILE)) return parse_while_stmt();
    if (match(lexer::TokenType::FOR)) return parse_for_stmt();
    if (match(lexer::TokenType::RETURN)) return parse_return_stmt();
    if (match(lexer::TokenType::BREAK)) return std::make_unique<ast::BreakStmt>();
    if (match(lexer::TokenType::CONTINUE)) return std::make_unique<ast::ContinueStmt>();
    
    return parse_assignment_or_expr();
}

std::unique_ptr<ast::FunctionDecl> Parser::parse_function_decl() {
    consume(lexer::TokenType::IDENTIFIER, "Expected function name");
    std::string name = previous().lexeme;
    
    consume(lexer::TokenType::LPAREN, "Expected '(' after function name");
    
    std::vector<ast::Parameter> params;
    if (!check(lexer::TokenType::RPAREN)) {
        do {
            skip_newlines();
            consume(lexer::TokenType::IDENTIFIER, "Expected parameter name");
            params.push_back(ast::Parameter{previous().lexeme});
            skip_newlines();
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RPAREN, "Expected ')' after parameters");
    skip_newlines();
    
    auto body = parse_block();
    
    return std::make_unique<ast::FunctionDecl>(name, std::move(params), 
                                               std::move(body), false);
}

std::unique_ptr<ast::ClassDecl> Parser::parse_class_decl() {
    consume(lexer::TokenType::IDENTIFIER, "Expected class name");
    std::string name = previous().lexeme;

    skip_newlines();
    consume(lexer::TokenType::LBRACE, "Expected '{' after class name");
    skip_newlines();

    std::vector<ast::FieldDecl>                     fields;
    std::vector<std::unique_ptr<ast::FunctionDecl>> methods;

    while (!check(lexer::TokenType::RBRACE) && !is_at_end()) {
        skip_newlines();

        if (check(lexer::TokenType::RBRACE)) break;

        if (match(lexer::TokenType::FUNC)) {
            // Method declaration
            auto method = parse_function_decl();
            methods.push_back(std::move(method));

        } else if (check(lexer::TokenType::IDENTIFIER)) {
            // Field declaration:  name [: type] [= default]
            ast::FieldDecl field;

            // Field name
            consume(lexer::TokenType::IDENTIFIER, "Expected field name");
            field.name = previous().lexeme;

            // Optional type annotation:  ": type"
            if (match(lexer::TokenType::COLON)) {
                // Collect the type string (may be "Option[str]", "int", etc.)
                std::string type_str;

                consume(lexer::TokenType::IDENTIFIER, "Expected type name");
                type_str = previous().lexeme;

                // Handle generic types like Option[str]
                if (match(lexer::TokenType::LBRACKET)) {
                    type_str += "[";
                    consume(lexer::TokenType::IDENTIFIER, "Expected inner type");
                    type_str += previous().lexeme;
                    consume(lexer::TokenType::RBRACKET, "Expected ']'");
                    type_str += "]";
                }

                field.type_annotation = type_str;
            }

            // Optional default value:  "= expr"
            if (match(lexer::TokenType::EQUAL)) {
                field.default_value = parse_expression();
            }

            fields.push_back(std::move(field));

        } else {
            error_reporter_.syntax_error(current(),
                "Expected field or method declaration in class");
            throw std::runtime_error("Parse error");
        }

        // Allow newline or comma between members
        if (!match(lexer::TokenType::NEWLINE)) {
            match(lexer::TokenType::COMMA);
        }
        skip_newlines();
    }

    consume(lexer::TokenType::RBRACE, "Expected '}' after class body");

    // ✅ Matches new ClassDecl(name, fields, methods)
    return std::make_unique<ast::ClassDecl>(
        name,
        std::move(fields),
        std::move(methods)
    );
}

std::unique_ptr<ast::Block> Parser::parse_block() {
    consume(lexer::TokenType::LBRACE, "Expected '{'");
    skip_newlines();
    
    std::vector<std::unique_ptr<ast::Stmt>> statements;
    
    while (!check(lexer::TokenType::RBRACE) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
        skip_newlines();
    }
    
    consume(lexer::TokenType::RBRACE, "Expected '}'");
    
    return std::make_unique<ast::Block>(std::move(statements));
}

std::unique_ptr<ast::IfStmt> Parser::parse_if_stmt() {
    auto condition = parse_expression();
    skip_newlines();
    
    auto then_block = parse_block();
    skip_newlines();
    
    std::unique_ptr<ast::Block> else_block = nullptr;
    
    if (match(lexer::TokenType::ELIF)) {
        auto elif_stmt = parse_if_stmt();
        std::vector<std::unique_ptr<ast::Stmt>> elif_vec;
        elif_vec.push_back(std::move(elif_stmt));
        else_block = std::make_unique<ast::Block>(std::move(elif_vec));
    } else if (match(lexer::TokenType::ELSE)) {
        skip_newlines();
        else_block = parse_block();
    }
    
    return std::make_unique<ast::IfStmt>(std::move(condition), 
                                        std::move(then_block), 
                                        std::move(else_block));
}

std::unique_ptr<ast::WhileStmt> Parser::parse_while_stmt() {
    auto condition = parse_expression();
    skip_newlines();
    auto body = parse_block();
    
    return std::make_unique<ast::WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<ast::ForStmt> Parser::parse_for_stmt() {
    consume(lexer::TokenType::IDENTIFIER, "Expected variable name in for loop");
    std::string variable = previous().lexeme;
    
    consume(lexer::TokenType::IN, "Expected 'in' in for loop");
    
    auto iterable = parse_expression();
    skip_newlines();
    auto body = parse_block();
    
    return std::make_unique<ast::ForStmt>(variable, std::move(iterable), 
                                         std::move(body));
}

std::unique_ptr<ast::ReturnStmt> Parser::parse_return_stmt() {
    std::unique_ptr<ast::Expr> value = nullptr;
    
    if (!check(lexer::TokenType::NEWLINE) && !check(lexer::TokenType::RBRACE) && !is_at_end()) {
        value = parse_expression();
    }
    
    return std::make_unique<ast::ReturnStmt>(std::move(value));
}

    std::unique_ptr<ast::Stmt> Parser::parse_assignment_or_expr() {
    // Parse the left-hand side (could be identifier or member access)
    auto expr = parse_expression();

    // Check if this is an assignment
    if (match(lexer::TokenType::EQUAL)) {
        // Get the value being assigned
        auto value = parse_expression();

        // Check what kind of assignment this is
        if (auto* ident = dynamic_cast<ast::Identifier*>(expr.get())) {
            // Simple variable assignment: x = 5
            std::string name = ident->name;
            return std::make_unique<ast::Assignment>(name, std::move(value));

        } else if (auto* member = dynamic_cast<ast::MemberExpr*>(expr.get())) {
            // Field assignment: obj.field = 5
            // For now, we need a new AST node for this
            // Create a FieldAssignment node (you'll need to add this to ast.h)
            return std::make_unique<ast::FieldAssignment>(
                std::move(member->object),
                member->member,
                std::move(value)
            );

        } else if (auto* index = dynamic_cast<ast::IndexExpr*>(expr.get())) {
            // Index assignment: arr[0] = 5
            return std::make_unique<ast::IndexAssignment>(
                std::move(index->object),
                std::move(index->index),
                std::move(value)
            );

        } else {
            error_reporter_.syntax_error(current(), "Invalid assignment target");
            throw std::runtime_error("Parse error");
        }
    }

    // Not an assignment, just an expression statement
    return std::make_unique<ast::ExprStmt>(std::move(expr));
}

// ============================================================================
// Expression Parsing (Precedence Climbing)
// ============================================================================

std::unique_ptr<ast::Expr> Parser::parse_expression() {
    return parse_logical_or();
}

std::unique_ptr<ast::Expr> Parser::parse_logical_or() {
    auto left = parse_logical_and();
    
    while (match(lexer::TokenType::OR)) {
        auto right = parse_logical_and();
        left = std::make_unique<ast::BinaryOp>(
            ast::BinaryOp::Op::OR, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ast::Expr> Parser::parse_logical_and() {
    auto left = parse_equality();
    
    while (match(lexer::TokenType::AND)) {
        auto right = parse_equality();
        left = std::make_unique<ast::BinaryOp>(
            ast::BinaryOp::Op::AND, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ast::Expr> Parser::parse_equality() {
    auto left = parse_comparison();
    
    while (match({lexer::TokenType::EQUAL_EQUAL, lexer::TokenType::NOT_EQUAL})) {
        auto op = previous().type == lexer::TokenType::EQUAL_EQUAL
            ? ast::BinaryOp::Op::EQUAL : ast::BinaryOp::Op::NOT_EQUAL;
        auto right = parse_comparison();
        left = std::make_unique<ast::BinaryOp>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ast::Expr> Parser::parse_comparison() {
    auto left = parse_term();
    
    while (match({lexer::TokenType::LESS, lexer::TokenType::LESS_EQUAL,
                 lexer::TokenType::GREATER, lexer::TokenType::GREATER_EQUAL})) {
        ast::BinaryOp::Op op;
        switch (previous().type) {
            case lexer::TokenType::LESS: op = ast::BinaryOp::Op::LESS; break;
            case lexer::TokenType::LESS_EQUAL: op = ast::BinaryOp::Op::LESS_EQUAL; break;
            case lexer::TokenType::GREATER: op = ast::BinaryOp::Op::GREATER; break;
            case lexer::TokenType::GREATER_EQUAL: op = ast::BinaryOp::Op::GREATER_EQUAL; break;
            default: op = ast::BinaryOp::Op::EQUAL; break;
        }
        auto right = parse_term();
        left = std::make_unique<ast::BinaryOp>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ast::Expr> Parser::parse_term() {
    auto left = parse_factor();
    
    while (match({lexer::TokenType::PLUS, lexer::TokenType::MINUS})) {
        auto op = previous().type == lexer::TokenType::PLUS
            ? ast::BinaryOp::Op::ADD : ast::BinaryOp::Op::SUB;
        auto right = parse_factor();
        left = std::make_unique<ast::BinaryOp>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ast::Expr> Parser::parse_factor() {
    auto left = parse_unary();
    
    while (match({lexer::TokenType::STAR, lexer::TokenType::SLASH,
                 lexer::TokenType::PERCENT, lexer::TokenType::DOUBLE_SLASH})) {
        ast::BinaryOp::Op op;
        switch (previous().type) {
            case lexer::TokenType::STAR: op = ast::BinaryOp::Op::MUL; break;
            case lexer::TokenType::SLASH: op = ast::BinaryOp::Op::DIV; break;
            case lexer::TokenType::PERCENT: op = ast::BinaryOp::Op::MOD; break;
            case lexer::TokenType::DOUBLE_SLASH: op = ast::BinaryOp::Op::FLOOR_DIV; break;
            default: op = ast::BinaryOp::Op::MUL; break;
        }
        auto right = parse_unary();
        left = std::make_unique<ast::BinaryOp>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ast::Expr> Parser::parse_unary() {
    if (match({lexer::TokenType::MINUS, lexer::TokenType::NOT})) {
        auto op = previous().type == lexer::TokenType::MINUS
            ? ast::UnaryOp::Op::NEG : ast::UnaryOp::Op::NOT;
        auto operand = parse_unary();
        return std::make_unique<ast::UnaryOp>(op, std::move(operand));
    }
    
    return parse_power();
}

std::unique_ptr<ast::Expr> Parser::parse_power() {
    auto left = parse_postfix();
    
    if (match(lexer::TokenType::DOUBLE_STAR)) {
        auto right = parse_unary();
        left = std::make_unique<ast::BinaryOp>(
            ast::BinaryOp::Op::POW, std::move(left), std::move(right));
    }
    
    return left;
}

    std::unique_ptr<ast::Expr> Parser::parse_postfix() {
    auto expr = parse_primary();

    while (true) {
        if (match(lexer::TokenType::LPAREN)) {
            // Check if this looks like an initializer (Type(name: value, ...))
            bool has_named_args = false;

            // Peek ahead to see if first arg is named
            if (check(lexer::TokenType::IDENTIFIER)) {
                size_t saved = current_;
                advance();  // skip identifier
                has_named_args = check(lexer::TokenType::COLON);
                current_ = saved;  // restore position
            }

            if (has_named_args && dynamic_cast<ast::Identifier*>(expr.get())) {
                // This is an initializer: Type(name: value, ...)
                std::string type_name = dynamic_cast<ast::Identifier*>(expr.get())->name;
                std::vector<ast::NamedArg> named_args;

                if (!check(lexer::TokenType::RPAREN)) {
                    do {
                        consume(lexer::TokenType::IDENTIFIER, "Expected argument name");
                        std::string arg_name = previous().lexeme;

                        consume(lexer::TokenType::COLON, "Expected ':' after argument name");

                        auto value = parse_expression();

                        ast::NamedArg arg;
                        arg.name = arg_name;
                        arg.value = std::move(value);
                        named_args.push_back(std::move(arg));

                    } while (match(lexer::TokenType::COMMA));
                }

                consume(lexer::TokenType::RPAREN, "Expected ')' after arguments");
                return std::make_unique<ast::InitializerExpr>(type_name, std::move(named_args));

            } else {
                // Regular function call
                std::vector<std::unique_ptr<ast::Expr>> args;

                if (!check(lexer::TokenType::RPAREN)) {
                    do {
                        args.push_back(parse_expression());
                    } while (match(lexer::TokenType::COMMA));
                }

                consume(lexer::TokenType::RPAREN, "Expected ')' after arguments");
                expr = std::make_unique<ast::CallExpr>(std::move(expr), std::move(args));
            }

        } else if (match(lexer::TokenType::LBRACKET)) {
            auto index = parse_expression();
            consume(lexer::TokenType::RBRACKET, "Expected ']' after index");
            expr = std::make_unique<ast::IndexExpr>(std::move(expr), std::move(index));

        } else if (match(lexer::TokenType::DOT)) {
            consume(lexer::TokenType::IDENTIFIER, "Expected member name after '.'");
            std::string member = previous().lexeme;
            expr = std::make_unique<ast::MemberExpr>(std::move(expr), member);

        } else {
            break;
        }
    }

    return expr;
}

    /*
std::unique_ptr<ast::Expr> Parser::parse_postfix() {
    auto expr = parse_primary();
    
    while (true) {
        if (match(lexer::TokenType::LPAREN)) {
            std::vector<std::unique_ptr<ast::Expr>> arguments;
            
            if (!check(lexer::TokenType::RPAREN)) {
                do {
                    skip_newlines();
                    arguments.push_back(parse_expression());
                    skip_newlines();
                } while (match(lexer::TokenType::COMMA));
            }
            
            consume(lexer::TokenType::RPAREN, "Expected ')' after arguments");
            expr = std::make_unique<ast::CallExpr>(std::move(expr), 
                                                   std::move(arguments));
            
        } else if (match(lexer::TokenType::LBRACKET)) {
            auto index = parse_expression();
            consume(lexer::TokenType::RBRACKET, "Expected ']' after index");
            expr = std::make_unique<ast::IndexExpr>(std::move(expr), std::move(index));
            
        } else if (match(lexer::TokenType::DOT)) {
            consume(lexer::TokenType::IDENTIFIER, "Expected member name after '.'");
            std::string member = previous().lexeme;
            expr = std::make_unique<ast::MemberExpr>(std::move(expr), member);
            
        } else {
            break;
        }
    }
    
    return expr;
}
*/

std::unique_ptr<ast::Expr> Parser::parse_primary() {
    if (match(lexer::TokenType::INTEGER)) {
        return std::make_unique<ast::IntegerLiteral>(
            std::get<long long>(previous().value));
    }
    
    if (match(lexer::TokenType::FLOAT)) {
        return std::make_unique<ast::FloatLiteral>(
            std::get<double>(previous().value));
    }
    
    if (match(lexer::TokenType::STRING)) {
        return std::make_unique<ast::StringLiteral>(
            std::get<std::string>(previous().value));
    }
    
    if (match(lexer::TokenType::TRUE)) {
        return std::make_unique<ast::BoolLiteral>(true);
    }
    
    if (match(lexer::TokenType::FALSE)) {
        return std::make_unique<ast::BoolLiteral>(false);
    }
    
    if (match(lexer::TokenType::NONE)) {
        return std::make_unique<ast::NoneLiteral>();
    }
    
    if (match(lexer::TokenType::IDENTIFIER)) {
        return std::make_unique<ast::Identifier>(previous().lexeme);
    }
    
    if (match(lexer::TokenType::LPAREN)) {
        auto expr = parse_expression();
        consume(lexer::TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    if (match(lexer::TokenType::LBRACKET)) {
        return parse_list_literal();
    }
    
    if (check(lexer::TokenType::LBRACE)) {
        return parse_dict_literal();
    }
    
    error_reporter_.syntax_error(current(), 
        "unexpected '" + current().lexeme + "'");
    throw std::runtime_error("Parse error");
}

std::unique_ptr<ast::Expr> Parser::parse_list_literal() {
    std::vector<std::unique_ptr<ast::Expr>> elements;
    skip_newlines();
    
    if (!check(lexer::TokenType::RBRACKET)) {
        do {
            skip_newlines();
            elements.push_back(parse_expression());
            skip_newlines();
        } while (match(lexer::TokenType::COMMA));
    }
    
    skip_newlines();
    consume(lexer::TokenType::RBRACKET, "Expected ']' after list elements");
    
    return std::make_unique<ast::ListExpr>(std::move(elements));
}

std::unique_ptr<ast::Expr> Parser::parse_dict_literal() {
    consume(lexer::TokenType::LBRACE, "Expected '{'");
    
    std::vector<std::pair<std::unique_ptr<ast::Expr>, 
                          std::unique_ptr<ast::Expr>>> pairs;
    skip_newlines();
    
    if (!check(lexer::TokenType::RBRACE)) {
        do {
            skip_newlines();
            auto key = parse_expression();
            consume(lexer::TokenType::COLON, "Expected ':' after dictionary key");
            auto value = parse_expression();
            pairs.emplace_back(std::move(key), std::move(value));
            skip_newlines();
        } while (match(lexer::TokenType::COMMA));
    }
    
    skip_newlines();
    consume(lexer::TokenType::RBRACE, "Expected '}' after dictionary elements");
    
    return std::make_unique<ast::DictExpr>(std::move(pairs));
}


std::unique_ptr<ast::StructDecl> Parser::parse_struct_decl() {
    // 'struct' keyword already consumed by parse_statement()

    // Struct name
    consume(lexer::TokenType::IDENTIFIER, "Expected struct name");
    std::string name = previous().lexeme;

    skip_newlines();
    consume(lexer::TokenType::LBRACE, "Expected '{' after struct name");
    skip_newlines();

    std::vector<ast::FieldDecl> fields;

    while (!check(lexer::TokenType::RBRACE) && !is_at_end()) {
        skip_newlines();
        if (check(lexer::TokenType::RBRACE)) break;

        // Each line is a field:  name [: type] [= default]
        consume(lexer::TokenType::IDENTIFIER, "Expected field name");
        ast::FieldDecl field;
        field.name = previous().lexeme;

        // Optional type annotation:  ": type"  or  ": Option[type]"
        if (match(lexer::TokenType::COLON)) {
            consume(lexer::TokenType::IDENTIFIER, "Expected type name after ':'");
            std::string type_str = previous().lexeme;

            // Handle generic e.g. Option[str]
            if (match(lexer::TokenType::LBRACKET)) {
                type_str += "[";
                consume(lexer::TokenType::IDENTIFIER, "Expected inner type");
                type_str += previous().lexeme;
                consume(lexer::TokenType::RBRACKET, "Expected ']' after inner type");
                type_str += "]";
            }

            field.type_annotation = type_str;
        }

        // Optional default value:  "= expr"
        if (match(lexer::TokenType::EQUAL)) {
            field.default_value = parse_expression();
        }

        // A field needs at least a type annotation OR a default value
        if (!field.type_annotation && !field.default_value) {
            error_reporter_.syntax_error(
                current(),
                "Field '" + field.name + "' must have a type annotation or default value"
            );
            throw std::runtime_error("Parse error");
        }

        fields.push_back(std::move(field));

        // Allow newline or comma between fields
        if (!match(lexer::TokenType::NEWLINE)) {
            match(lexer::TokenType::COMMA);
        }
        skip_newlines();
    }

    consume(lexer::TokenType::RBRACE, "Expected '}' after struct body");

    return std::make_unique<ast::StructDecl>(name, std::move(fields));
}


// ============================================================================
// Main Parse Method
// ============================================================================

std::unique_ptr<ast::Module> Parser::parse() {
    std::vector<std::unique_ptr<ast::Stmt>> statements;
    
    skip_newlines();
    
    while (!is_at_end()) {
        try {
            auto stmt = parse_statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const std::runtime_error&) {
            return nullptr;  // Error already reported
        }
        skip_newlines();
    }
    
    return std::make_unique<ast::Module>(std::move(statements));
}

}