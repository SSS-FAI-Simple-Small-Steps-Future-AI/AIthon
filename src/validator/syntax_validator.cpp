//
// Created by Furqan on 12/02/2026.
//

#include "../../include/validator/syntax_validator.h"
#include <sstream>
#include <iostream>

namespace aithon::validator {

SyntaxValidator::ValidationResult SyntaxValidator::validate(const std::string& source_code) {
    ValidationResult result;
    result.is_valid = true;

    // Run all validation checks
    check_def_keyword(source_code, result.errors);
    check_colon_terminators(source_code, result.errors);
    check_indentation_blocks(source_code, result.errors);
    check_missing_braces(source_code, result.errors);
    check_unmatched_braces(source_code, result.errors);

    // If any errors found, validation fails
    if (!result.errors.empty()) {
        result.is_valid = false;
    }

    return result;
}

void SyntaxValidator::check_def_keyword(const std::string& source_code,
                                       std::vector<ValidationError>& errors) {
    // Look for 'def' keyword (word boundary aware)
    std::regex def_regex(R"(\bdef\s+\w+\s*\()");
    std::smatch match;

    std::string::const_iterator search_start(source_code.cbegin());
    while (std::regex_search(search_start, source_code.cend(), match, def_regex)) {
        size_t pos = match.position() + (search_start - source_code.cbegin());

        // Skip if inside string or comment
        if (is_inside_string(source_code, pos) || is_inside_comment(source_code, pos)) {
            search_start = match.suffix().first;
            continue;
        }

        ValidationError error;
        error.line_number = get_line_number(source_code, pos);
        error.column = get_column_number(source_code, pos);
        error.error_type = "INVALID_KEYWORD";
        error.message = "'def' keyword is not allowed in PyVM";
        error.suggestion = "Replace 'def' with 'func'";
        error.code_snippet = get_code_snippet(source_code, pos);

        errors.push_back(error);

        search_start = match.suffix().first;
    }
}

void SyntaxValidator::check_colon_terminators(const std::string& source_code,
                                              std::vector<ValidationError>& errors) {
    // Look for colons that terminate function/if/while/for statements
    // Patterns: 'func name():' or 'if condition:' or 'while condition:' etc.

    std::vector<std::regex> patterns = {
        std::regex(R"(\bfunc\s+\w+\s*\([^)]*\)\s*:)"),      // func name():
        std::regex(R"(\bif\s+.+:\s*$)", std::regex::multiline),        // if condition:
        std::regex(R"(\belif\s+.+:\s*$)", std::regex::multiline),      // elif condition:
        std::regex(R"(\belse\s*:\s*$)", std::regex::multiline),        // else:
        std::regex(R"(\bwhile\s+.+:\s*$)", std::regex::multiline),     // while condition:
        std::regex(R"(\bfor\s+.+:\s*$)", std::regex::multiline),       // for item in items:
        std::regex(R"(\btry\s*:\s*$)", std::regex::multiline),         // try:
        std::regex(R"(\bexcept\s+.*:\s*$)", std::regex::multiline),    // except Exception:
        std::regex(R"(\bfinally\s*:\s*$)", std::regex::multiline),     // finally:
        std::regex(R"(\bwith\s+.+:\s*$)", std::regex::multiline),      // with resource:
    };

    for (auto& pattern : patterns) {
        std::smatch match;
        std::string::const_iterator search_start(source_code.cbegin());

        while (std::regex_search(search_start, source_code.cend(), match, pattern)) {
            size_t pos = match.position() + (search_start - source_code.cbegin());

            // Skip if inside string or comment
            if (is_inside_string(source_code, pos) || is_inside_comment(source_code, pos)) {
                search_start = match.suffix().first;
                continue;
            }

            // Find the colon position
            std::string matched_text = match.str();
            size_t colon_pos = matched_text.rfind(':');
            if (colon_pos != std::string::npos) {
                size_t actual_colon_pos = pos + colon_pos;

                ValidationError error;
                error.line_number = get_line_number(source_code, actual_colon_pos);
                error.column = get_column_number(source_code, actual_colon_pos);
                error.error_type = "INVALID_TERMINATOR";
                error.message = "Colon ':' terminator is not allowed in PyVM";
                error.suggestion = "Replace ':' with '{' and add closing '}'";
                error.code_snippet = get_code_snippet(source_code, actual_colon_pos);

                errors.push_back(error);
            }

            search_start = match.suffix().first;
        }
    }
}

void SyntaxValidator::check_indentation_blocks(const std::string& source_code,
                                               std::vector<ValidationError>& errors) {
    std::istringstream iss(source_code);
    std::string line;
    int line_number = 0;
    int prev_indent = 0;
    bool expect_block = false;

    while (std::getline(iss, line)) {
        line_number++;

        // Skip empty lines and comments
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }

        std::string trimmed = line;
        size_t first_char = trimmed.find_first_not_of(" \t");
        if (first_char != std::string::npos) {
            trimmed = trimmed.substr(first_char);
        }

        if (trimmed[0] == '#') {
            continue;  // Comment line
        }

        // Count leading spaces/tabs
        int indent = 0;
        for (char c : line) {
            if (c == ' ') indent++;
            else if (c == '\t') indent += 4;
            else break;
        }

        // If we expected a block and got indentation without '{', error
        if (expect_block && !line.empty()) {
            // Check if line has opening brace
            if (line.find('{') == std::string::npos) {
                // Check if indentation increased (Python-style)
                if (indent > prev_indent) {
                    ValidationError error;
                    error.line_number = line_number;
                    error.column = 0;
                    error.error_type = "INDENTATION_NOT_ALLOWED";
                    error.message = "Indentation-based blocks are not allowed in PyVM";
                    error.suggestion = "Use curly braces '{ }' to delimit blocks";
                    error.code_snippet = line;

                    errors.push_back(error);
                }
            }
            expect_block = false;
        }

        // Check if this line expects a block (ends with statement keyword)
        if (trimmed.find("func ") == 0 ||
            trimmed.find("if ") == 0 ||
            trimmed.find("elif ") == 0 ||
            trimmed.find("else") == 0 ||
            trimmed.find("while ") == 0 ||
            trimmed.find("for ") == 0 ||
            trimmed.find("try") == 0 ||
            trimmed.find("except") == 0 ||
            trimmed.find("finally") == 0 ||
            trimmed.find("with ") == 0) {

            // If no '{' on this line, expect it on next
            if (line.find('{') == std::string::npos) {
                expect_block = true;
            }
        }

        prev_indent = indent;
    }
}

void SyntaxValidator::check_missing_braces(const std::string& source_code,
                                          std::vector<ValidationError>& errors) {
    // Check for statements that should have braces but don't
    std::vector<std::pair<std::regex, std::string>> patterns = {
        {std::regex(R"(func\s+\w+\s*\([^)]*\)\s*$)", std::regex::multiline), "func"},
        {std::regex(R"(if\s+.+\s*$)", std::regex::multiline), "if"},
        {std::regex(R"(elif\s+.+\s*$)", std::regex::multiline), "elif"},
        {std::regex(R"(else\s*$)", std::regex::multiline), "else"},
        {std::regex(R"(while\s+.+\s*$)", std::regex::multiline), "while"},
        {std::regex(R"(for\s+.+\s*$)", std::regex::multiline), "for"},
    };

    for (auto& [pattern, keyword] : patterns) {
        std::smatch match;
        std::string::const_iterator search_start(source_code.cbegin());

        while (std::regex_search(search_start, source_code.cend(), match, pattern)) {
            size_t pos = match.position() + (search_start - source_code.cbegin());

            // Skip if inside string or comment
            if (is_inside_string(source_code, pos) || is_inside_comment(source_code, pos)) {
                search_start = match.suffix().first;
                continue;
            }

            std::string matched = match.str();

            // Check if it doesn't have '{' and doesn't have ':'
            if (matched.find('{') == std::string::npos &&
                matched.find(':') == std::string::npos) {

                ValidationError error;
                error.line_number = get_line_number(source_code, pos);
                error.column = get_column_number(source_code, pos);
                error.error_type = "MISSING_BRACE";
                error.message = "Missing opening brace '{' after " + keyword + " statement";
                error.suggestion = "Add '{ }' to delimit the block";
                error.code_snippet = get_code_snippet(source_code, pos);

                errors.push_back(error);
            }

            search_start = match.suffix().first;
        }
    }
}

void SyntaxValidator::check_unmatched_braces(const std::string& source_code,
                                            std::vector<ValidationError>& errors) {
    std::vector<size_t> open_braces;

    for (size_t i = 0; i < source_code.size(); i++) {
        // Skip strings and comments
        if (is_inside_string(source_code, i) || is_inside_comment(source_code, i)) {
            continue;
        }

        if (source_code[i] == '{') {
            open_braces.push_back(i);
        } else if (source_code[i] == '}') {
            if (open_braces.empty()) {
                ValidationError error;
                error.line_number = get_line_number(source_code, i);
                error.column = get_column_number(source_code, i);
                error.error_type = "UNMATCHED_BRACE";
                error.message = "Closing brace '}' without matching opening brace '{'";
                error.suggestion = "Check brace pairing";
                error.code_snippet = get_code_snippet(source_code, i);

                errors.push_back(error);
            } else {
                open_braces.pop_back();
            }
        }
    }

    // Check for unclosed braces
    for (size_t pos : open_braces) {
        ValidationError error;
        error.line_number = get_line_number(source_code, pos);
        error.column = get_column_number(source_code, pos);
        error.error_type = "UNCLOSED_BRACE";
        error.message = "Opening brace '{' without matching closing brace '}'";
        error.suggestion = "Add closing '}' for this block";
        error.code_snippet = get_code_snippet(source_code, pos);

        errors.push_back(error);
    }
}

int SyntaxValidator::get_line_number(const std::string& text, size_t pos) {
    int line = 1;
    for (size_t i = 0; i < pos && i < text.size(); i++) {
        if (text[i] == '\n') line++;
    }
    return line;
}

int SyntaxValidator::get_column_number(const std::string& text, size_t pos) {
    int col = 1;
    for (size_t i = pos; i > 0 && i < text.size(); i--) {
        if (text[i - 1] == '\n') break;
        col++;
    }
    return col;
}

std::string SyntaxValidator::get_code_snippet(const std::string& text, size_t pos) {
    // Find start of line
    size_t line_start = pos;
    while (line_start > 0 && text[line_start - 1] != '\n') {
        line_start--;
    }

    // Find end of line
    size_t line_end = pos;
    while (line_end < text.size() && text[line_end] != '\n') {
        line_end++;
    }

    return text.substr(line_start, line_end - line_start);
}

bool SyntaxValidator::is_inside_string(const std::string& text, size_t pos) {
    bool in_single = false;
    bool in_double = false;
    bool escaped = false;

    for (size_t i = 0; i < pos && i < text.size(); i++) {
        if (escaped) {
            escaped = false;
            continue;
        }

        if (text[i] == '\\') {
            escaped = true;
            continue;
        }

        if (text[i] == '\'' && !in_double) {
            in_single = !in_single;
        } else if (text[i] == '"' && !in_single) {
            in_double = !in_double;
        }
    }

    return in_single || in_double;
}

bool SyntaxValidator::is_inside_comment(const std::string& text, size_t pos) {
    // Find start of line
    size_t line_start = pos;
    while (line_start > 0 && text[line_start - 1] != '\n') {
        line_start--;
    }

    // Check if there's a # before pos on this line (not in string)
    for (size_t i = line_start; i < pos; i++) {
        if (text[i] == '#' && !is_inside_string(text, i)) {
            return true;
        }
    }

    return false;
}

}