#pragma once

#include "token.h"
#include "../utils/error_reporter.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace aithon::lexer {

    class Lexer {
    private:
        std::string source_;
        utils::ErrorReporter& error_reporter_;
        size_t current_;
        int line_;
        int column_;
        int line_start_;

        static const std::unordered_map<std::string, TokenType> keywords_;

        // Helper methods
        [[nodiscard]] bool is_at_end() const;
        [[nodiscard]] char peek() const;
        char peek_next() const;
        char advance();
        bool match(char expected);
        [[nodiscard]] SourceLocation current_location() const;

        void skip_whitespace();
        void skip_comment();

        Token make_number();
        Token make_string(char quote);
        Token make_identifier();

    public:
        Lexer(std::string source, utils::ErrorReporter& reporter);
        std::vector<Token> tokenize();
    };

}