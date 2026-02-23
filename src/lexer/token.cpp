#include "../../include/lexer/token.h"

namespace aithon::lexer {

const char* token_type_to_string(const TokenType type) {
    switch (type) {
        case TokenType::INTEGER:        return "INTEGER";
        case TokenType::FLOAT:          return "FLOAT";
        case TokenType::STRING:         return "STRING";
        case TokenType::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::FUNC:           return "'func'";
        case TokenType::STRUCT:         return "'struct'";
        case TokenType::CLASS:          return "'class'";
        case TokenType::IF:             return "'if'";
        case TokenType::ELIF:           return "'elif'";
        case TokenType::ELSE:           return "'else'";
        case TokenType::WHILE:          return "'while'";
        case TokenType::FOR:            return "'for'";
        case TokenType::IN:             return "'in'";
        case TokenType::RETURN:         return "'return'";
        case TokenType::BREAK:          return "'break'";
        case TokenType::CONTINUE:       return "'continue'";
        case TokenType::AND:            return "'and'";
        case TokenType::OR:             return "'or'";
        case TokenType::NOT:            return "'not'";
        case TokenType::TRUE:           return "'True'";
        case TokenType::FALSE:          return "'False'";
        case TokenType::NONE:           return "'None'";
        case TokenType::LPAREN:         return "'('";
        case TokenType::RPAREN:         return "')'";
        case TokenType::LBRACE:         return "'{'";
        case TokenType::RBRACE:         return "'}'";
        case TokenType::LBRACKET:       return "'['";
        case TokenType::RBRACKET:       return "']'";
        case TokenType::EQUAL:          return "'='";
        case TokenType::EQUAL_EQUAL:    return "'=='";
        case TokenType::NOT_EQUAL:      return "'!='";
        case TokenType::LESS:           return "'<'";
        case TokenType::GREATER:        return "'>'";
        case TokenType::PLUS:           return "'+'";
        case TokenType::MINUS:          return "'-'";
        case TokenType::STAR:           return "'*'";
        case TokenType::SLASH:          return "'/'";
        case TokenType::COLON:          return "':'";
        case TokenType::COMMA:          return "','";
        case TokenType::DOT:            return "'.'";
        case TokenType::SEMICOLON:      return "';'";
        case TokenType::LESS_EQUAL:     return "'<='";
        case TokenType::GREATER_EQUAL:  return "'>='";
        case TokenType::PERCENT:        return "'%'";
        case TokenType::DOUBLE_SLASH:   return "'//'";
        case TokenType::DOUBLE_STAR:    return "'**'";
        case TokenType::NEWLINE:        return "newline";
        case TokenType::END_OF_FILE:    return "end of file";
        case TokenType::ERROR:          return "error";
        default: return "unknown token";
    }
}

}