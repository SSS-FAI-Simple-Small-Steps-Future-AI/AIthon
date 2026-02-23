#include "../../include/utils/error_reporter.h"
#include <iostream>
#include <sstream>

namespace aithon::utils {

ErrorReporter::ErrorReporter(std::string source, std::string filename)
    : source_code_(std::move(source)), 
      filename_(std::move(filename)),
      has_errors_(false) {
    split_source_lines();
}

void ErrorReporter::split_source_lines() {
    std::istringstream stream(source_code_);
    std::string line;
    while (std::getline(stream, line)) {
        source_lines_.push_back(line);
    }
}

void ErrorReporter::syntax_error(const lexer::SourceLocation& loc, const std::string& message) {
    has_errors_ = true;
    
    std::cerr << "  File \"" << filename_ << "\", line " << loc.line << "\n";
    
    // Print the source line
    if (loc.line > 0 && loc.line <= static_cast<int>(source_lines_.size())) {
        std::string line = source_lines_[loc.line - 1];
        std::cerr << "    " << line << "\n";
        
        // Print the caret pointing to the error
        std::cerr << "    ";
        for (int i = 1; i < loc.column; i++) {
            std::cerr << " ";
        }
        std::cerr << "^\n";
    }
    
    std::cerr << "SyntaxError: " << message << "\n\n";
}

void ErrorReporter::syntax_error(const lexer::Token& token, const std::string& message) {
    syntax_error(token.location, message);
}

void ErrorReporter::syntax_error_expected(const lexer::SourceLocation& loc,
                                         const std::string& expected,
                                         const std::string& got) {
    std::string msg = "expected " + expected + ", got " + got;
    syntax_error(loc, msg);
}

void ErrorReporter::lexer_error(const lexer::SourceLocation& loc, const std::string& message) {
    has_errors_ = true;

    std::cerr << "  File \"" << filename_ << "\", line " << loc.line << "\n";

    if (loc.line > 0 && loc.line <= static_cast<int>(source_lines_.size())) {
        std::string line = source_lines_[loc.line - 1];
        std::cerr << "    " << line << "\n";
        std::cerr << "    ";
        for (int i = 1; i < loc.column; i++) {
            std::cerr << " ";
        }
        std::cerr << "^\n";
    }

    std::cerr << "LexerError: " << message << "\n\n";
}

}