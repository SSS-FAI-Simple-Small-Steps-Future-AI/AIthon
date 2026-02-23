#pragma once

#include "../lexer/token.h"
#include <string>
#include <vector>

namespace aithon::utils {

    class ErrorReporter {
    private:
        std::string source_code_;
        std::string filename_;
        std::vector<std::string> source_lines_;
        bool has_errors_;

        void split_source_lines();

    public:
        explicit ErrorReporter(std::string source, std::string filename = "<unknown>");

        void syntax_error(const lexer::SourceLocation& loc, const std::string& message);
        void syntax_error(const lexer::Token& token, const std::string& message);
        void syntax_error_expected(const lexer::SourceLocation& loc,
                                   const std::string& expected,
                                   const std::string& got);
        void lexer_error(const lexer::SourceLocation& loc, const std::string& message);

        [[nodiscard]] bool has_errors() const { return has_errors_; }
        void reset() { has_errors_ = false; }
    };

}