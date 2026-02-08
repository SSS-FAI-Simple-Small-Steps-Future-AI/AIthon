#include "../../include/runtime/exceptions.h"
#include <iostream>
#include <sstream>

namespace aithon::runtime {

ExceptionHandler* ExceptionContext::find_handler(const std::string& exception_type) {
    // Search from most recent to oldest handler
    for (auto it = handlers_.rbegin(); it != handlers_.rend(); ++it) {
        // Empty type means catch all
        if (it->exception_type.empty() || it->exception_type == exception_type) {
            return &(*it);
        }
    }
    return nullptr;
}

void ExceptionContext::add_traceback_entry(const std::string& function_name,
                                          const std::string& filename,
                                          int lineno) {
    std::ostringstream oss;
    oss << "  File \"" << filename << "\", line " << lineno 
        << ", in " << function_name;
    traceback_.push_back(oss.str());
    
    if (current_exception_) {
        current_exception_->add_traceback(oss.str());
    }
}

std::shared_ptr<PyException> make_exception(const std::string& type,
                                           const std::string& message) {
    if (type == "ValueError") {
        return std::make_shared<ValueError>(message);
    } else if (type == "TypeError") {
        return std::make_shared<TypeError>(message);
    } else if (type == "KeyError") {
        return std::make_shared<KeyError>(message);
    } else if (type == "IndexError") {
        return std::make_shared<IndexError>(message);
    } else if (type == "AttributeError") {
        return std::make_shared<AttributeError>(message);
    } else if (type == "RuntimeError") {
        return std::make_shared<RuntimeError>(message);
    } else if (type == "ZeroDivisionError") {
        return std::make_shared<ZeroDivisionError>(message);
    } else if (type == "StopIteration") {
        return std::make_shared<StopIteration>();
    } else if (type == "ImportError") {
        return std::make_shared<ImportError>(message);
    } else {
        return std::make_shared<PyException>(type, message);
    }
}

void print_exception(std::shared_ptr<PyException> exc) {
    std::cerr << "Traceback (most recent call last):\n";
    
    auto traceback = exc->traceback();
    for (const auto& frame : traceback) {
        std::cerr << frame << "\n";
    }
    
    std::cerr << exc->type_name() << ": " << exc->message() << "\n";
}

} // namespace pyvm::runtime