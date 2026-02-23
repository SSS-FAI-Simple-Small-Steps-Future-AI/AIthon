//
// Created by Furqan on 12/02/2026.
//

#pragma once

#include <string>
#include <vector>
#include <regex>
#include <sstream>

namespace aithon::validator {

/**
 * Strict Syntax Validator
 *
 * Enforces PyVM-only syntax rules:
 * 1. 'def' keyword is NOT allowed (must use 'func')
 * 2. Colon ':' terminators are NOT allowed (must use '{')
 * 3. Indentation-based blocks are NOT allowed (must use '{ }')
 * 4. All blocks MUST be delimited with curly braces
 */
class SyntaxValidator {
public:
    struct ValidationError {
        int line_number;
        int column;
        std::string error_type;
        std::string message;
        std::string suggestion;
        std::string code_snippet;

        // Add this method to format a single error
        [[nodiscard]] std::string to_string() const {
            std::ostringstream oss;
            oss << "[" << error_type << "] Line " << line_number << ":" << column
                << " - " << message;
            if (!suggestion.empty()) {
                oss << "\n    💡 " << suggestion;
            }
            if (!code_snippet.empty()) {
                oss << "\n    " << code_snippet;
            }
            return oss.str();
        }
    };

    struct ValidationResult {
        bool is_valid;
        std::vector<ValidationError> errors;
        std::vector<std::string> warnings;
    };

    /**
     * Validate source code against PyVM syntax rules
     */
    static ValidationResult validate(const std::string& source_code);

private:
    /**
     * Check for 'def' keyword usage
     */
    static void check_def_keyword(const std::string& source_code,
                                  std::vector<ValidationError>& errors);

    /**
     * Check for colon terminators (should use '{' instead)
     */
    static void check_colon_terminators(const std::string& source_code,
                                       std::vector<ValidationError>& errors);

    /**
     * Check for indentation-based blocks (should use '{ }')
     */
    static void check_indentation_blocks(const std::string& source_code,
                                        std::vector<ValidationError>& errors);

    /**
     * Check for missing curly braces
     */
    static void check_missing_braces(const std::string& source_code,
                                    std::vector<ValidationError>& errors);

    /**
     * Check for unmatched braces
     */
    static void check_unmatched_braces(const std::string& source_code,
                                      std::vector<ValidationError>& errors);

    /**
     * Get line number for position in string
     */
    static int get_line_number(const std::string& text, size_t pos);

    /**
     * Get column number for position in string
     */
    static int get_column_number(const std::string& text, size_t pos);

    /**
     * Get code snippet around position
     */
    static std::string get_code_snippet(const std::string& text, size_t pos);

    /**
     * Check if position is inside a string literal
     */
    static bool is_inside_string(const std::string& text, size_t pos);

    /**
     * Check if position is inside a comment
     */
    static bool is_inside_comment(const std::string& text, size_t pos);
};

}


inline std::ostream& operator<<(std::ostream& os, const std::vector<aithon::validator::SyntaxValidator::ValidationError>& errors) {
    if (errors.empty()) {
        os << "No errors";
        return os;
    }

    os << "Found " << errors.size() << " error(s):\n";
    for (size_t i = 0; i < errors.size(); i++) {
        os << "\n" << (i + 1) << ". " << errors[i].to_string();
    }
    return os;
}