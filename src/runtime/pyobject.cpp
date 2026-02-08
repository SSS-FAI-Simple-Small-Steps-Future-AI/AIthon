#include "runtime/pyobject.h"
#include <sstream>
#include <cmath>

namespace aithon::runtime {

// ============================================================================
// PyObject Base Implementation
// ============================================================================

PyObject* PyObject::add(PyObject* other) {
    throw std::runtime_error("TypeError: unsupported operand type(s) for +");
}

PyObject* PyObject::sub(PyObject* other) {
    throw std::runtime_error("TypeError: unsupported operand type(s) for -");
}

PyObject* PyObject::mul(PyObject* other) {
    throw std::runtime_error("TypeError: unsupported operand type(s) for *");
}

PyObject* PyObject::div(PyObject* other) {
    throw std::runtime_error("TypeError: unsupported operand type(s) for /");
}

PyObject* PyObject::mod(PyObject* other) {
    throw std::runtime_error("TypeError: unsupported operand type(s) for %");
}

PyObject* PyObject::pow(PyObject* other) {
    throw std::runtime_error("TypeError: unsupported operand type(s) for **");
}

PyObject* PyObject::eq(PyObject* other) {
    if (this == other) return  PyBool::get(true);
    return  PyBool::get(false);
    // return new PyBool(this == other);
}

PyObject* PyObject::ne(PyObject* other) {
    if (this != other) return  PyBool::get(true);
    return  PyBool::get(false);
    // return new PyBool(this != other);
}

PyObject* PyObject::lt(PyObject* other) {
    throw std::runtime_error("TypeError: '<' not supported between instances");
}

PyObject* PyObject::le(PyObject* other) {
    throw std::runtime_error("TypeError: '<=' not supported between instances");
}

PyObject* PyObject::gt(PyObject* other) {
    throw std::runtime_error("TypeError: '>' not supported between instances");
}

PyObject* PyObject::ge(PyObject* other) {
    throw std::runtime_error("TypeError: '>=' not supported between instances");
}

PyObject* PyObject::neg() {
    throw std::runtime_error("TypeError: bad operand type for unary -");
}

PyObject* PyObject::pos() {
    return this;
}

PyObject* PyObject::invert() {
    throw std::runtime_error("TypeError: bad operand type for unary ~");
}

PyObject* PyObject::get_item(PyObject* key) {
    throw std::runtime_error("TypeError: object is not subscriptable");
}

void PyObject::set_item(PyObject* key, PyObject* value) {
    throw std::runtime_error("TypeError: object does not support item assignment");
}

PyObject* PyObject::call(PyObject** args, size_t nargs) {
    throw std::runtime_error("TypeError: object is not callable");
}

std::string PyObject::to_string() const {
    std::ostringstream oss;
    oss << "<object at " << this << ">";
    return oss.str();
}

bool PyObject::to_bool() const {
    return true; // Most objects are truthy by default
}

int64_t PyObject::hash() const {
    return reinterpret_cast<int64_t>(this);
}

// ============================================================================
// PyNone Implementation
// ============================================================================

PyNone* PyNone::instance() {
    static PyNone none_instance;
    return &none_instance;
}

std::string PyNone::to_string() const {
    return "None";
}

bool PyNone::to_bool() const {
    return false;
}

// ============================================================================
// PyBool Implementation
// ============================================================================

PyBool* PyBool::get(bool value) {
    static PyBool true_instance(true);
    static PyBool false_instance(false);
    return value ? &true_instance : &false_instance;
}

std::string PyBool::to_string() const {
    return value_ ? "True" : "False";
}

bool PyBool::to_bool() const {
    return value_;
}

PyObject* PyBool::eq(PyObject* other) {
    if (other->is_bool()) {
        return PyBool::get(value_ == static_cast<PyBool*>(other)->value());
    }
    return PyBool::get(false);
}

// ============================================================================
// PyInt Implementation
// ============================================================================

PyObject* PyInt::add(PyObject* other) {
    if (other->is_int()) {
        return new PyInt(value_ + static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return new PyFloat(value_ + static_cast<PyFloat*>(other)->value());
    }
    return PyObject::add(other);
}

PyObject* PyInt::sub(PyObject* other) {
    if (other->is_int()) {
        return new PyInt(value_ - static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return new PyFloat(value_ - static_cast<PyFloat*>(other)->value());
    }
    return PyObject::sub(other);
}

PyObject* PyInt::mul(PyObject* other) {
    if (other->is_int()) {
        return new PyInt(value_ * static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return new PyFloat(value_ * static_cast<PyFloat*>(other)->value());
    } else if (other->is_string()) {
        // String * int
        PyString* str = static_cast<PyString*>(other);
        std::string result;
        for (int64_t i = 0; i < value_; i++) {
            result += str->value();
        }
        return new PyString(result);
    } else if (other->is_list()) {
        // List * int
        PyList* list = static_cast<PyList*>(other);
        PyList* result = new PyList();
        for (int64_t i = 0; i < value_; i++) {
            for (size_t j = 0; j < list->size(); j++) {
                result->append(list->get(j));
            }
        }
        return result;
    }
    return PyObject::mul(other);
}

PyObject* PyInt::div(PyObject* other) {
    if (other->is_int()) {
        int64_t divisor = static_cast<PyInt*>(other)->value();
        if (divisor == 0) {
            throw std::runtime_error("ZeroDivisionError: division by zero");
        }
        return new PyFloat(static_cast<double>(value_) / divisor);
    } else if (other->is_float()) {
        double divisor = static_cast<PyFloat*>(other)->value();
        if (divisor == 0.0) {
            throw std::runtime_error("ZeroDivisionError: division by zero");
        }
        return new PyFloat(value_ / divisor);
    }
    return PyObject::div(other);
}

PyObject* PyInt::mod(PyObject* other) {
    if (other->is_int()) {
        int64_t divisor = static_cast<PyInt*>(other)->value();
        if (divisor == 0) {
            throw std::runtime_error("ZeroDivisionError: integer modulo by zero");
        }
        return new PyInt(value_ % divisor);
    }
    return PyObject::mod(other);
}

PyObject* PyInt::pow(PyObject* other) {
    if (other->is_int()) {
        int64_t exponent = static_cast<PyInt*>(other)->value();
        return new PyInt(std::pow(value_, exponent));
    } else if (other->is_float()) {
        double exponent = static_cast<PyFloat*>(other)->value();
        return new PyFloat(std::pow(value_, exponent));
    }
    return PyObject::pow(other);
}

PyObject* PyInt::eq(PyObject* other) {
    if (other->is_int()) {
        return PyBool::get(value_ == static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return PyBool::get(value_ == static_cast<PyFloat*>(other)->value());
    }
    return PyBool::get(false);
}

PyObject* PyInt::ne(PyObject* other) {
    if (other->is_int()) {
        return PyBool::get(value_ != static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return PyBool::get(value_ != static_cast<PyFloat*>(other)->value());
    }
    return PyBool::get(true);
}

PyObject* PyInt::lt(PyObject* other) {
    if (other->is_int()) {
        return PyBool::get(value_ < static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return PyBool::get(value_ < static_cast<PyFloat*>(other)->value());
    }
    return PyObject::lt(other);
}

PyObject* PyInt::le(PyObject* other) {
    if (other->is_int()) {
        return PyBool::get(value_ <= static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return PyBool::get(value_ <= static_cast<PyFloat*>(other)->value());
    }
    return PyObject::le(other);
}

PyObject* PyInt::gt(PyObject* other) {
    if (other->is_int()) {
        return PyBool::get(value_ > static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return PyBool::get(value_ > static_cast<PyFloat*>(other)->value());
    }
    return PyObject::gt(other);
}

PyObject* PyInt::ge(PyObject* other) {
    if (other->is_int()) {
        return PyBool::get(value_ >= static_cast<PyInt*>(other)->value());
    } else if (other->is_float()) {
        return PyBool::get(value_ >= static_cast<PyFloat*>(other)->value());
    }
    return PyObject::ge(other);
}

PyObject* PyInt::neg() {
    return new PyInt(-value_);
}

std::string PyInt::to_string() const {
    return std::to_string(value_);
}

bool PyInt::to_bool() const {
    return value_ != 0;
}

int64_t PyInt::hash() const {
    return value_;
}

// ============================================================================
// PyFloat Implementation
// ============================================================================

PyObject* PyFloat::add(PyObject* other) {
    if (other->is_float()) {
        return new PyFloat(value_ + static_cast<PyFloat*>(other)->value());
    } else if (other->is_int()) {
        return new PyFloat(value_ + static_cast<PyInt*>(other)->value());
    }
    return PyObject::add(other);
}

PyObject* PyFloat::sub(PyObject* other) {
    if (other->is_float()) {
        return new PyFloat(value_ - static_cast<PyFloat*>(other)->value());
    } else if (other->is_int()) {
        return new PyFloat(value_ - static_cast<PyInt*>(other)->value());
    }
    return PyObject::sub(other);
}

PyObject* PyFloat::mul(PyObject* other) {
    if (other->is_float()) {
        return new PyFloat(value_ * static_cast<PyFloat*>(other)->value());
    } else if (other->is_int()) {
        return new PyFloat(value_ * static_cast<PyInt*>(other)->value());
    }
    return PyObject::mul(other);
}

PyObject* PyFloat::div(PyObject* other) {
    double divisor = 0.0;
    if (other->is_float()) {
        divisor = static_cast<PyFloat*>(other)->value();
    } else if (other->is_int()) {
        divisor = static_cast<PyInt*>(other)->value();
    } else {
        return PyObject::div(other);
    }

    if (divisor == 0.0) {
        throw std::runtime_error("ZeroDivisionError: float division by zero");
    }
    return new PyFloat(value_ / divisor);
}

PyObject* PyFloat::eq(PyObject* other) {
    if (other->is_float()) {
        return PyBool::get(value_ == static_cast<PyFloat*>(other)->value());
    } else if (other->is_int()) {
        return PyBool::get(value_ == static_cast<PyInt*>(other)->value());
    }
    return PyBool::get(false);
}

PyObject* PyFloat::lt(PyObject* other) {
    if (other->is_float()) {
        return PyBool::get(value_ < static_cast<PyFloat*>(other)->value());
    } else if (other->is_int()) {
        return PyBool::get(value_ < static_cast<PyInt*>(other)->value());
    }
    return PyObject::lt(other);
}

PyObject* PyFloat::neg() {
    return new PyFloat(-value_);
}

std::string PyFloat::to_string() const {
    return std::to_string(value_);
}

bool PyFloat::to_bool() const {
    return value_ != 0.0;
}

// ============================================================================
// PyString Implementation
// ============================================================================

PyObject* PyString::add(PyObject* other) {
    if (other->is_string()) {
        return new PyString(value_ + static_cast<PyString*>(other)->value());
    }
    throw std::runtime_error("TypeError: can only concatenate str (not \"" +
                           std::string(typeid(*other).name()) + "\") to str");
}

PyObject* PyString::mul(PyObject* other) {
    if (other->is_int()) {
        int64_t count = static_cast<PyInt*>(other)->value();
        std::string result;
        result.reserve(value_.size() * count);
        for (int64_t i = 0; i < count; i++) {
            result += value_;
        }
        return new PyString(result);
    }
    throw std::runtime_error("TypeError: can't multiply sequence by non-int");
}

PyObject* PyString::eq(PyObject* other) {
    if (other->is_string()) {
        return PyBool::get(value_ == static_cast<PyString*>(other)->value());
    }
    return PyBool::get(false);
}

PyObject* PyString::get_item(PyObject* key) {
    if (!key->is_int()) {
        throw std::runtime_error("TypeError: string indices must be integers");
    }

    int64_t index = static_cast<PyInt*>(key)->value();
    if (index < 0) {
        index += value_.size();
    }

    if (index < 0 || index >= static_cast<int64_t>(value_.size())) {
        throw std::runtime_error("IndexError: string index out of range");
    }

    return new PyString(std::string(1, value_[index]));
}

std::string PyString::to_string() const {
    return value_;
}

bool PyString::to_bool() const {
    return !value_.empty();
}

size_t PyString::length() const {
    return value_.size();
}

int64_t PyString::hash() const {
    return std::hash<std::string>{}(value_);
}

// ============================================================================
// PyList Implementation
// ============================================================================

void PyList::append(PyObject* item) {
    items_.push_back(item);
    if (item) item->incref();
}

PyObject* PyList::get(size_t index) const {
    if (index >= items_.size()) {
        throw std::runtime_error("IndexError: list index out of range");
    }
    return items_[index];
}

void PyList::set(size_t index, PyObject* value) {
    if (index >= items_.size()) {
        throw std::runtime_error("IndexError: list assignment index out of range");
    }

    PyObject* old = items_[index];
    items_[index] = value;

    if (value) value->incref();
    if (old) old->decref();
}

PyObject* PyList::get_item(PyObject* key) {
    if (!key->is_int()) {
        throw std::runtime_error("TypeError: list indices must be integers");
    }

    int64_t index = static_cast<PyInt*>(key)->value();
    if (index < 0) {
        index += items_.size();
    }

    if (index < 0 || index >= static_cast<int64_t>(items_.size())) {
        throw std::runtime_error("IndexError: list index out of range");
    }

    return items_[index];
}

void PyList::set_item(PyObject* key, PyObject* value) {
    if (!key->is_int()) {
        throw std::runtime_error("TypeError: list indices must be integers");
    }

    int64_t index = static_cast<PyInt*>(key)->value();
    if (index < 0) {
        index += items_.size();
    }

    set(index, value);
}

std::string PyList::to_string() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < items_.size(); i++) {
        if (i > 0) oss << ", ";
        if (items_[i]) {
            oss << items_[i]->to_string();
        } else {
            oss << "None";
        }
    }
    oss << "]";
    return oss.str();
}

bool PyList::to_bool() const {
    return !items_.empty();
}

size_t PyList::length() const {
    return items_.size();
}

// ============================================================================
// PyDict Implementation
// ============================================================================

void PyDict::set(const std::string& key, PyObject* value) {
    auto it = items_.find(key);
    if (it != items_.end()) {
        PyObject* old = it->second;
        it->second = value;
        if (value) value->incref();
        if (old) old->decref();
    } else {
        items_[key] = value;
        if (value) value->incref();
    }
}

PyObject* PyDict::get(const std::string& key) const {
    auto it = items_.find(key);
    if (it != items_.end()) {
        return it->second;
    }
    return nullptr;
}

PyObject* PyDict::get_item(PyObject* key) {
    std::string key_str;
    if (key->is_string()) {
        key_str = static_cast<PyString*>(key)->value();
    } else {
        key_str = key->to_string();
    }

    PyObject* value = get(key_str);
    if (!value) {
        throw std::runtime_error("KeyError: '" + key_str + "'");
    }
    return value;
}

void PyDict::set_item(PyObject* key, PyObject* value) {
    std::string key_str;
    if (key->is_string()) {
        key_str = static_cast<PyString*>(key)->value();
    } else {
        key_str = key->to_string();
    }

    set(key_str, value);
}

std::string PyDict::to_string() const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : items_) {
        if (!first) oss << ", ";
        oss << "'" << key << "': ";
        if (value) {
            oss << value->to_string();
        } else {
            oss << "None";
        }
        first = false;
    }
    oss << "}";
    return oss.str();
}

bool PyDict::to_bool() const {
    return !items_.empty();
}

size_t PyDict::length() const {
    return items_.size();
}

} // namespace pyvm::runtime