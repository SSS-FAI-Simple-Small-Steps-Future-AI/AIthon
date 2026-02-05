#pragma once

#include "pyobject.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace pyvm::runtime {

// Exception handler structure
struct ExceptionHandler {
    std::string exception_type;  // Empty means catch all
    void* handler_block;         // LLVM basic block for handler
    void* finally_block;         // Optional finally block
    int stack_depth;             // For unwinding
};

// Exception context for try/except/finally
class ExceptionContext {
private:
    std::vector<ExceptionHandler> handlers_;
    std::shared_ptr<PyException> current_exception_;
    std::vector<std::string> traceback_;
    bool in_finally_;
    
public:
    ExceptionContext() : in_finally_(false) {}
    
    // Register exception handler
    void push_handler(const ExceptionHandler& handler) {
        handlers_.push_back(handler);
    }
    
    // Remove exception handler
    void pop_handler() {
        if (!handlers_.empty()) {
            handlers_.pop_back();
        }
    }
    
    // Find handler for exception type
    ExceptionHandler* find_handler(const std::string& exception_type);
    
    // Set current exception
    void set_exception(std::shared_ptr<PyException> exc) {
        current_exception_ = exc;
    }
    
    std::shared_ptr<PyException> get_exception() {
        return current_exception_;
    }
    
    void clear_exception() {
        current_exception_ = nullptr;
    }
    
    bool has_exception() const {
        return current_exception_ != nullptr;
    }
    
    // Traceback management
    void add_traceback_entry(const std::string& function_name, 
                            const std::string& filename,
                            int lineno);
    
    std::vector<std::string> get_traceback() const {
        return traceback_;
    }
    
    void clear_traceback() {
        traceback_.clear();
    }
    
    // Finally block handling
    void enter_finally() { in_finally_ = true; }
    void exit_finally() { in_finally_ = false; }
    bool in_finally() const { return in_finally_; }
};

// Built-in exception types
class ValueError : public PyException {
public:
    explicit ValueError(const std::string& message)
        : PyException("ValueError", message) {}
};

class TypeError : public PyException {
public:
    explicit TypeError(const std::string& message)
        : PyException("TypeError", message) {}
};

class KeyError : public PyException {
public:
    explicit KeyError(const std::string& message)
        : PyException("KeyError", message) {}
};

class IndexError : public PyException {
public:
    explicit IndexError(const std::string& message)
        : PyException("IndexError", message) {}
};

class AttributeError : public PyException {
public:
    explicit AttributeError(const std::string& message)
        : PyException("AttributeError", message) {}
};

class RuntimeError : public PyException {
public:
    explicit RuntimeError(const std::string& message)
        : PyException("RuntimeError", message) {}
};

class ZeroDivisionError : public PyException {
public:
    explicit ZeroDivisionError(const std::string& message)
        : PyException("ZeroDivisionError", message) {}
};

class StopIteration : public PyException {
public:
    StopIteration() : PyException("StopIteration", "") {}
};

class ImportError : public PyException {
public:
    explicit ImportError(const std::string& message)
        : PyException("ImportError", message) {}
};

// Exception utilities
std::shared_ptr<PyException> make_exception(const std::string& type, 
                                           const std::string& message);

void print_exception(std::shared_ptr<PyException> exc);

// Macro for throwing exceptions from C++ code
#define PYVM_THROW(ExceptionType, message) \
    throw ExceptionType(message)

#define PYVM_CHECK_TYPE(obj, expected_type, operation) \
    if ((obj)->type() != (expected_type)) { \
        throw TypeError("Unsupported operand type for " operation); \
    }

} // namespace pyvm::runtime