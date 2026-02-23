#pragma once

#include <string>
#include <variant>

namespace aithon::lexer {

enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    TRUE,
    FALSE,
    NONE,
    
    // Identifiers and keywords
    IDENTIFIER,
    FUNC,
    STRUCT,
    CLASS,
    IF,
    ELIF,
    ELSE,
    WHILE,
    FOR,
    IN,
    RETURN,
    BREAK,
    CONTINUE,
    AND,
    OR,
    NOT,
    
    // Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    DOUBLE_SLASH,   // //
    DOUBLE_STAR,    // **
    
    EQUAL,          // =
    EQUAL_EQUAL,    // ==
    NOT_EQUAL,      // !=
    LESS,           // <
    LESS_EQUAL,     // <=
    GREATER,        // >
    GREATER_EQUAL,  // >=
    
    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    COMMA,          // ,
    COLON,          // :
    DOT,            // .
    SEMICOLON,      // ;
    
    // Special
    NEWLINE,
    END_OF_FILE,
    ERROR
};

struct SourceLocation {
    int line;
    int column;
    int offset;
    
    SourceLocation(int l = 1, int c = 1, int o = 0) 
        : line(l), column(c), offset(o) {}
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;
    std::variant<long long, double, std::string> value;
    
    Token(TokenType t, std::string lex, SourceLocation loc)
        : type(t), lexeme(std::move(lex)), location(loc) {}
    
    Token(TokenType t, std::string lex, SourceLocation loc, long long val)
        : type(t), lexeme(std::move(lex)), location(loc), value(val) {}
    
    Token(TokenType t, std::string lex, SourceLocation loc, double val)
        : type(t), lexeme(std::move(lex)), location(loc), value(val) {}
    
    Token(TokenType t, std::string lex, SourceLocation loc, std::string val)
        : type(t), lexeme(std::move(lex)), location(loc), value(std::move(val)) {}
};

// Helper function declaration
const char* token_type_to_string(TokenType type);

}