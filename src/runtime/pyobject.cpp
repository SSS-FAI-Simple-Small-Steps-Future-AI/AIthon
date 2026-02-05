#include "include/runtime/pyobject.h"
#include <sstream>
#include <stdexcept>

namespace aithon::runtime {

// PyObject base implementations
std::shared_ptr<PyObject> PyObject::add(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: + for " + to_string());
}

std::shared_ptr<PyObject> PyObject::sub(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: - for " + to_string());
}

std::shared_ptr<PyObject> PyObject::mul(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: * for " + to_string());
}

std::shared_ptr<PyObject> PyObject::div(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: / for " + to_string());
}

std::shared_ptr<PyObject> PyObject::mod(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: % for " + to_string());
}

std::shared_ptr<PyObject> PyObject::eq(std::shared_ptr<PyObject> other) {
    return make_bool(this == other.get());
}

std::shared_ptr<PyObject> PyObject::ne(std::shared_ptr<PyObject> other) {
    return make_bool(this != other.get());
}

std::shared_ptr<PyObject> PyObject::lt(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: < for " + to_string());
}

std::shared_ptr<PyObject> PyObject::le(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: <= for " + to_string());
}

std::shared_ptr<PyObject> PyObject::gt(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: > for " + to_string());
}

std::shared_ptr<PyObject> PyObject::ge(std::shared_ptr<PyObject> other) {
    throw std::runtime_error("Unsupported operation: >= for " + to_string());
}

std::shared_ptr<PyObject> PyObject::getitem(std::shared_ptr<PyObject> key) {
    throw std::runtime_error("Object is not subscriptable");
}

void PyObject::setitem(std::shared_ptr<PyObject> key, std::shared_ptr<PyObject> value) {
    throw std::runtime_error("Object does not support item assignment");
}

// PyBool implementations
std::shared_ptr<PyObject> PyBool::eq(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::BOOL) {
        auto other_bool = std::static_pointer_cast<PyBool>(other);
        return make_bool(value_ == other_bool->value());
    }
    return make_bool(false);
}

// PyInt implementations
std::shared_ptr<PyObject> PyInt::add(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_int(value_ + other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_float(value_ + other_float->value());
    }
    throw std::runtime_error("Unsupported operand type for +");
}

std::shared_ptr<PyObject> PyInt::sub(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_int(value_ - other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_float(value_ - other_float->value());
    }
    throw std::runtime_error("Unsupported operand type for -");
}

std::shared_ptr<PyObject> PyInt::mul(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_int(value_ * other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_float(value_ * other_float->value());
    } else if (other->type() == PyType::STRING) {
        auto str = std::static_pointer_cast<PyString>(other);
        return str->mul(shared_from_this());
    } else if (other->type() == PyType::LIST) {
        auto list = std::static_pointer_cast<PyList>(other);
        return list->mul(shared_from_this());
    }
    throw std::runtime_error("Unsupported operand type for *");
}

std::shared_ptr<PyObject> PyInt::div(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        if (other_int->value() == 0) {
            throw std::runtime_error("Division by zero");
        }
        return make_float(static_cast<double>(value_) / other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        if (other_float->value() == 0.0) {
            throw std::runtime_error("Division by zero");
        }
        return make_float(value_ / other_float->value());
    }
    throw std::runtime_error("Unsupported operand type for /");
}

std::shared_ptr<PyObject> PyInt::mod(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        if (other_int->value() == 0) {
            throw std::runtime_error("Modulo by zero");
        }
        return make_int(value_ % other_int->value());
    }
    throw std::runtime_error("Unsupported operand type for %");
}

std::shared_ptr<PyObject> PyInt::eq(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_bool(value_ == other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_bool(value_ == other_float->value());
    }
    return make_bool(false);
}

std::shared_ptr<PyObject> PyInt::ne(std::shared_ptr<PyObject> other) {
    auto result = eq(other);
    return make_bool(!std::static_pointer_cast<PyBool>(result)->value());
}

std::shared_ptr<PyObject> PyInt::lt(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_bool(value_ < other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_bool(value_ < other_float->value());
    }
    throw std::runtime_error("Unsupported operand type for <");
}

std::shared_ptr<PyObject> PyInt::le(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_bool(value_ <= other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_bool(value_ <= other_float->value());
    }
    throw std::runtime_error("Unsupported operand type for <=");
}

std::shared_ptr<PyObject> PyInt::gt(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_bool(value_ > other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_bool(value_ > other_float->value());
    }
    throw std::runtime_error("Unsupported operand type for >");
}

std::shared_ptr<PyObject> PyInt::ge(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_bool(value_ >= other_int->value());
    } else if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_bool(value_ >= other_float->value());
    }
    throw std::runtime_error("Unsupported operand type for >=");
}

// PyFloat implementations
std::shared_ptr<PyObject> PyFloat::add(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_float(value_ + other_float->value());
    } else if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_float(value_ + other_int->value());
    }
    throw std::runtime_error("Unsupported operand type for +");
}

std::shared_ptr<PyObject> PyFloat::sub(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_float(value_ - other_float->value());
    } else if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_float(value_ - other_int->value());
    }
    throw std::runtime_error("Unsupported operand type for -");
}

std::shared_ptr<PyObject> PyFloat::mul(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_float(value_ * other_float->value());
    } else if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_float(value_ * other_int->value());
    }
    throw std::runtime_error("Unsupported operand type for *");
}

std::shared_ptr<PyObject> PyFloat::div(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        if (other_float->value() == 0.0) {
            throw std::runtime_error("Division by zero");
        }
        return make_float(value_ / other_float->value());
    } else if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        if (other_int->value() == 0) {
            throw std::runtime_error("Division by zero");
        }
        return make_float(value_ / other_int->value());
    }
    throw std::runtime_error("Unsupported operand type for /");
}

std::shared_ptr<PyObject> PyFloat::eq(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_bool(value_ == other_float->value());
    } else if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_bool(value_ == other_int->value());
    }
    return make_bool(false);
}

std::shared_ptr<PyObject> PyFloat::lt(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::FLOAT) {
        auto other_float = std::static_pointer_cast<PyFloat>(other);
        return make_bool(value_ < other_float->value());
    } else if (other->type() == PyType::INT) {
        auto other_int = std::static_pointer_cast<PyInt>(other);
        return make_bool(value_ < other_int->value());
    }
    throw std::runtime_error("Unsupported operand type for <");
}

// PyString implementations
std::shared_ptr<PyObject> PyString::add(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::STRING) {
        auto other_str = std::static_pointer_cast<PyString>(other);
        return make_string(value_ + other_str->value());
    }
    throw std::runtime_error("Can only concatenate str (not \"" + 
                           other->to_string() + "\") to str");
}

std::shared_ptr<PyObject> PyString::mul(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto count = std::static_pointer_cast<PyInt>(other);
        std::string result;
        for (int64_t i = 0; i < count->value(); i++) {
            result += value_;
        }
        return make_string(result);
    }
    throw std::runtime_error("Can't multiply sequence by non-int");
}

std::shared_ptr<PyObject> PyString::eq(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::STRING) {
        auto other_str = std::static_pointer_cast<PyString>(other);
        return make_bool(value_ == other_str->value());
    }
    return make_bool(false);
}

std::shared_ptr<PyObject> PyString::getitem(std::shared_ptr<PyObject> key) {
    if (key->type() == PyType::INT) {
        auto index = std::static_pointer_cast<PyInt>(key);
        int64_t idx = index->value();
        if (idx < 0) idx += value_.size();
        if (idx < 0 || idx >= static_cast<int64_t>(value_.size())) {
            throw std::runtime_error("String index out of range");
        }
        return make_string(std::string(1, value_[idx]));
    }
    throw std::runtime_error("String indices must be integers");
}

// PyList implementations
std::string PyList::to_string() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < items_.size(); i++) {
        if (i > 0) oss << ", ";
        oss << items_[i]->to_string();
    }
    oss << "]";
    return oss.str();
}

void PyList::insert(size_t index, std::shared_ptr<PyObject> item) {
    if (index > items_.size()) index = items_.size();
    items_.insert(items_.begin() + index, item);
}

void PyList::remove(size_t index) {
    if (index < items_.size()) {
        items_.erase(items_.begin() + index);
    }
}

std::shared_ptr<PyObject> PyList::getitem(std::shared_ptr<PyObject> key) {
    if (key->type() == PyType::INT) {
        auto index = std::static_pointer_cast<PyInt>(key);
        int64_t idx = index->value();
        if (idx < 0) idx += items_.size();
        if (idx < 0 || idx >= static_cast<int64_t>(items_.size())) {
            throw std::runtime_error("List index out of range");
        }
        return items_[idx];
    }
    throw std::runtime_error("List indices must be integers");
}

void PyList::setitem(std::shared_ptr<PyObject> key, std::shared_ptr<PyObject> value) {
    if (key->type() == PyType::INT) {
        auto index = std::static_pointer_cast<PyInt>(key);
        int64_t idx = index->value();
        if (idx < 0) idx += items_.size();
        if (idx < 0 || idx >= static_cast<int64_t>(items_.size())) {
            throw std::runtime_error("List assignment index out of range");
        }
        items_[idx] = value;
        return;
    }
    throw std::runtime_error("List indices must be integers");
}

std::shared_ptr<PyObject> PyList::add(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::LIST) {
        auto other_list = std::static_pointer_cast<PyList>(other);
        auto result = std::make_shared<PyList>(items_);
        for (const auto& item : other_list->items()) {
            result->append(item);
        }
        return result;
    }
    throw std::runtime_error("Can only concatenate list to list");
}

std::shared_ptr<PyObject> PyList::mul(std::shared_ptr<PyObject> other) {
    if (other->type() == PyType::INT) {
        auto count = std::static_pointer_cast<PyInt>(other);
        auto result = std::make_shared<PyList>();
        for (int64_t i = 0; i < count->value(); i++) {
            for (const auto& item : items_) {
                result->append(item);
            }
        }
        return result;
    }
    throw std::runtime_error("Can't multiply sequence by non-int");
}

// PyDict implementations
std::string PyDict::to_string() const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : items_) {
        if (!first) oss << ", ";
        oss << "'" << key << "': " << value->to_string();
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::shared_ptr<PyObject> PyDict::getitem(std::shared_ptr<PyObject> key) {
    std::string key_str;
    if (key->type() == PyType::STRING) {
        key_str = std::static_pointer_cast<PyString>(key)->value();
    } else {
        key_str = key->to_string();
    }
    
    auto it = items_.find(key_str);
    if (it != items_.end()) {
        return it->second;
    }
    throw std::runtime_error("KeyError: '" + key_str + "'");
}

void PyDict::setitem(std::shared_ptr<PyObject> key, std::shared_ptr<PyObject> value) {
    std::string key_str;
    if (key->type() == PyType::STRING) {
        key_str = std::static_pointer_cast<PyString>(key)->value();
    } else {
        key_str = key->to_string();
    }
    items_[key_str] = value;
}

// PyClass implementations
std::shared_ptr<PyFunction> PyClass::get_method(const std::string& name) {
    auto it = methods_.find(name);
    if (it != methods_.end()) {
        return it->second;
    }
    
    // Check bases
    for (auto& base : bases_) {
        auto method = base->get_method(name);
        if (method) return method;
    }
    
    return nullptr;
}

std::shared_ptr<PyObject> PyClass::call(const std::vector<std::shared_ptr<PyObject>>& args) {
    auto instance = std::make_shared<PyInstance>(
        std::static_pointer_cast<PyClass>(shared_from_this())
    );
    
    // Call __init__ if it exists
    auto init_method = get_method("__init__");
    if (init_method) {
        std::vector<std::shared_ptr<PyObject>> init_args = {instance};
        init_args.insert(init_args.end(), args.begin(), args.end());
        init_method->call(init_args);
    }
    
    return instance;
}

// PyInstance implementations
std::shared_ptr<PyObject> PyInstance::get_attr(const std::string& name) {
    // Check instance attributes first
    if (has_attr(name)) {
        return PyObject::get_attr(name);
    }
    
    // Check class methods
    auto method = class_->get_method(name);
    if (method) {
        // Bind method to instance
        return method;
    }
    
    throw std::runtime_error("AttributeError: '" + class_->name() + 
                           "' object has no attribute '" + name + "'");
}

// PyGenerator implementations
std::shared_ptr<PyObject> PyGenerator::next() {
    if (state_ == State::COMPLETED) {
        throw std::runtime_error("StopIteration");
    }
    
    // Resume generator execution
    // This would need integration with the runtime/codegen
    state_ = State::RUNNING;
    
    // ... execute until next yield ...
    
    state_ = State::SUSPENDED;
    return current_value_;
}

void PyGenerator::send(std::shared_ptr<PyObject> value) {
    current_value_ = value;
    // Resume execution with value
}

// Helper functions
std::shared_ptr<PyObject> make_int(int64_t value) {
    return std::make_shared<PyInt>(value);
}

std::shared_ptr<PyObject> make_float(double value) {
    return std::make_shared<PyFloat>(value);
}

std::shared_ptr<PyObject> make_string(const std::string& value) {
    return std::make_shared<PyString>(value);
}

std::shared_ptr<PyObject> make_bool(bool value) {
    return std::make_shared<PyBool>(value);
}

std::shared_ptr<PyObject> make_list(const std::vector<std::shared_ptr<PyObject>>& items) {
    return std::make_shared<PyList>(items);
}

std::shared_ptr<PyObject> make_dict() {
    return std::make_shared<PyDict>();
}

std::shared_ptr<PyObject> make_none() {
    return PyNone::instance();
}

} // namespace aithon::runtime