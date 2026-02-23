#include "../../include/lexer/lexer.h"
#include <cctype>

namespace aithon::lexer {

// Static keyword map initialization
const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"func", TokenType::FUNC},
    {"struct", TokenType::STRUCT},
    {"class", TokenType::CLASS},
    {"if", TokenType::IF},
    {"elif", TokenType::ELIF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"return", TokenType::RETURN},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
    {"True", TokenType::TRUE},
    {"False", TokenType::FALSE},
    {"None", TokenType::NONE},
};

Lexer::Lexer(std::string source, utils::ErrorReporter& reporter)
    : source_(std::move(source)), 
      error_reporter_(reporter),
      current_(0),
      line_(1),
      column_(1),
      line_start_(0) {}

bool Lexer::is_at_end() const {
    return current_ >= source_.length();
}

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char Lexer::peek_next() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    column_++;
    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (source_[current_] != expected) return false;
    advance();
    return true;
}

SourceLocation Lexer::current_location() const {
    return {line_, column_, line_start_};
    // return SourceLocation(line_, column_, current_);
}

void Lexer::skip_whitespace() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skip_comment() {
    while (peek() != '\n' && !is_at_end()) {
        advance();
    }
}

Token Lexer::make_number() {
    SourceLocation start_loc = current_location();
    std::string num_str;
    
    while (std::isdigit(peek())) {
        num_str += advance();
    }
    
    // Check for decimal point
    if (peek() == '.' && std::isdigit(peek_next())) {
        num_str += advance(); // consume '.'
        
        while (std::isdigit(peek())) {
            num_str += advance();
        }
        
        // Float
        double value = std::stod(num_str);
        return {TokenType::FLOAT, num_str, start_loc, value};
    }
    
    // Integer
    long long value = std::stoll(num_str);
    return {TokenType::INTEGER, num_str, start_loc, value};
}

Token Lexer::make_string(char quote) {
    SourceLocation start_loc = current_location();
    std::string str_value;
    
    // Check for triple quotes
    bool is_triple = false;
    if (peek() == quote && peek_next() == quote) {
        is_triple = true;
        advance();
        advance();
    }
    
    while (!is_at_end()) {
        char c = peek();
        
        // Check for end of string
        if (c == quote) {
            if (is_triple) {
                if (peek_next() == quote && current_ + 2 < source_.length() && 
                    source_[current_ + 2] == quote) {
                    advance(); // consume first quote
                    advance(); // consume second quote
                    advance(); // consume third quote
                    break;
                }
            } else {
                advance(); // consume closing quote
                break;
            }
        }
        
        // Handle escape sequences
        if (c == '\\' && !is_triple) {
            advance();
            if (!is_at_end()) {
                c = advance();
                switch (c) {
                    case 'n': str_value += '\n'; break;
                    case 't': str_value += '\t'; break;
                    case 'r': str_value += '\r'; break;
                    case '\\': str_value += '\\'; break;
                    case '\'': str_value += '\''; break;
                    case '\"': str_value += '\"'; break;
                    default: str_value += c; break;
                }
            }
            continue;
        }
        
        if (c == '\n') {
            line_++;
            column_ = 0;
            line_start_ = current_ + 1;
        }
        
        str_value += advance();
    }
    
    std::string lexeme = is_triple ? R"(""")" + str_value + R"(""")" :
                                    std::string(1, quote) + str_value + std::string(1, quote);
    return {TokenType::STRING, lexeme, start_loc, str_value};
}

Token Lexer::make_identifier() {
    SourceLocation start_loc = current_location();
    std::string ident;
    
    while (std::isalnum(peek()) || peek() == '_') {
        ident += advance();
    }
    
    // Check if it's a keyword
    auto it = keywords_.find(ident);
    if (it != keywords_.end()) {
        return {it->second, ident, start_loc};
    }
    
    return {TokenType::IDENTIFIER, ident, start_loc};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!is_at_end()) {
        skip_whitespace();
        
        if (is_at_end()) break;
        
        SourceLocation loc = current_location();
        char c = peek();
        
        // Skip comments
        if (c == '#') {
            skip_comment();
            continue;
        }
        
        // Newlines
        if (c == '\n') {
            advance();
            tokens.emplace_back(TokenType::NEWLINE, "\\n", loc);
            line_++;
            column_ = 1;
            line_start_ = current_;
            continue;
        }
        
        // Numbers
        if (std::isdigit(c)) {
            tokens.push_back(make_number());
            continue;
        }
        
        // Strings
        if (c == '"' || c == '\'') {
            char quote = advance();
            tokens.push_back(make_string(quote));
            continue;
        }
        
        // Identifiers and keywords
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(make_identifier());
            continue;
        }
        
        // Two-character operators
        if (c == '=' && peek_next() == '=') {
            advance(); advance();
            tokens.emplace_back(TokenType::EQUAL_EQUAL, "==", loc);
            continue;
        }
        if (c == '!' && peek_next() == '=') {
            advance(); advance();
            tokens.emplace_back(TokenType::NOT_EQUAL, "!=", loc);
            continue;
        }
        if (c == '<' && peek_next() == '=') {
            advance(); advance();
            tokens.emplace_back(TokenType::LESS_EQUAL, "<=", loc);
            continue;
        }
        if (c == '>' && peek_next() == '=') {
            advance(); advance();
            tokens.emplace_back(TokenType::GREATER_EQUAL, ">=", loc);
            continue;
        }
        if (c == '/' && peek_next() == '/') {
            advance(); advance();
            tokens.emplace_back(TokenType::DOUBLE_SLASH, "//", loc);
            continue;
        }
        if (c == '*' && peek_next() == '*') {
            advance(); advance();
            tokens.emplace_back(TokenType::DOUBLE_STAR, "**", loc);
            continue;
        }
        
        // Single-character tokens
        switch (c) {
            case '+': advance(); tokens.emplace_back(TokenType::PLUS, "+", loc); break;
            case '-': advance(); tokens.emplace_back(TokenType::MINUS, "-", loc); break;
            case '*': advance(); tokens.emplace_back(TokenType::STAR, "*", loc); break;
            case '/': advance(); tokens.emplace_back(TokenType::SLASH, "/", loc); break;
            case '%': advance(); tokens.emplace_back(TokenType::PERCENT, "%", loc); break;
            case '=': advance(); tokens.emplace_back(TokenType::EQUAL, "=", loc); break;
            case '<': advance(); tokens.emplace_back(TokenType::LESS, "<", loc); break;
            case '>': advance(); tokens.emplace_back(TokenType::GREATER, ">", loc); break;
            case '(': advance(); tokens.emplace_back(TokenType::LPAREN, "(", loc); break;
            case ')': advance(); tokens.emplace_back(TokenType::RPAREN, ")", loc); break;
            case '{': advance(); tokens.emplace_back(TokenType::LBRACE, "{", loc); break;
            case '}': advance(); tokens.emplace_back(TokenType::RBRACE, "}", loc); break;
            case '[': advance(); tokens.emplace_back(TokenType::LBRACKET, "[", loc); break;
            case ']': advance(); tokens.emplace_back(TokenType::RBRACKET, "]", loc); break;
            case ',': advance(); tokens.emplace_back(TokenType::COMMA, ",", loc); break;
            case ':': advance(); tokens.emplace_back(TokenType::COLON, ":", loc); break;
            case '.': advance(); tokens.emplace_back(TokenType::DOT, ".", loc); break;
            case ';': advance(); tokens.emplace_back(TokenType::SEMICOLON, ";", loc); break;
            
            default:
                error_reporter_.lexer_error(loc, "unexpected character '" + std::string(1, c) + "'");
                advance();
                break;
        }
    }
    
    tokens.emplace_back(TokenType::END_OF_FILE, "", current_location());
    return tokens;
}

}