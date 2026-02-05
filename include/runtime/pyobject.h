#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>

namespace aithon::runtime {

// Forward declarations
class PyObject;
class PyString;
class PyList;
class PyDict;
class PyInt;
class PyFloat;
class PyBool;
class PyNone;
class PyFunction;
class PyClass;
class PyInstance;
class PyException;

// Type enumeration
enum class PyType {
    NONE,
    BOOL,
    INT,
    FLOAT,
    STRING,
    LIST,
    DICT,
    TUPLE,
    FUNCTION,
    CLASS,
    INSTANCE,
    EXCEPTION,
    GENERATOR,
    MODULE
};

// Base Python object
class PyObject {
protected:
    PyType type_;
    int ref_count_;
    std::unordered_map<std::string, std::shared_ptr<PyObject>> attributes_;
    
public:
    explicit PyObject(PyType type) : type_(type), ref_count_(1) {}
    virtual ~PyObject() = default;
    
    PyType type() const { return type_; }
    
    // Reference counting
    void incref() { ref_count_++; }
    void decref() { 
        if (--ref_count_ == 0) {
            delete this;
        }
    }
    int refcount() const { return ref_count_; }
    
    // Attribute access
    void set_attr(const std::string& name, std::shared_ptr<PyObject> value) {
        attributes_[name] = value;
    }
    
    std::shared_ptr<PyObject> get_attr(const std::string& name) {
        auto it = attributes_.find(name);
        if (it != attributes_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    bool has_attr(const std::string& name) const {
        return attributes_.find(name) != attributes_.end();
    }
    
    // Python operations
    virtual std::string to_string() const { return "<object>"; }
    virtual std::shared_ptr<PyObject> call(const std::vector<std::shared_ptr<PyObject>>& args) {
        throw std::runtime_error("Object is not callable");
    }
    virtual bool is_true() const { return true; }
    
    // Arithmetic operations (to be overridden)
    virtual std::shared_ptr<PyObject> add(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> sub(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> mul(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> div(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> mod(std::shared_ptr<PyObject> other);
    
    // Comparison operations
    virtual std::shared_ptr<PyObject> eq(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> ne(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> lt(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> le(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> gt(std::shared_ptr<PyObject> other);
    virtual std::shared_ptr<PyObject> ge(std::shared_ptr<PyObject> other);
    
    // Container operations
    virtual std::shared_ptr<PyObject> getitem(std::shared_ptr<PyObject> key);
    virtual void setitem(std::shared_ptr<PyObject> key, std::shared_ptr<PyObject> value);
    virtual size_t len() const { return 0; }
};

// None type
class PyNone : public PyObject {
public:
    PyNone() : PyObject(PyType::NONE) {}
    std::string to_string() const override { return "None"; }
    bool is_true() const override { return false; }
    
    static std::shared_ptr<PyNone> instance() {
        static auto none = std::make_shared<PyNone>();
        return none;
    }
};

// Boolean type
class PyBool : public PyObject {
private:
    bool value_;
    
public:
    explicit PyBool(bool value) : PyObject(PyType::BOOL), value_(value) {}
    
    bool value() const { return value_; }
    std::string to_string() const override { return value_ ? "True" : "False"; }
    bool is_true() const override { return value_; }
    
    std::shared_ptr<PyObject> eq(std::shared_ptr<PyObject> other) override;
};

// Integer type
class PyInt : public PyObject {
private:
    int64_t value_;
    
public:
    explicit PyInt(int64_t value) : PyObject(PyType::INT), value_(value) {}
    
    int64_t value() const { return value_; }
    std::string to_string() const override { return std::to_string(value_); }
    bool is_true() const override { return value_ != 0; }
    
    std::shared_ptr<PyObject> add(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> sub(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> mul(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> div(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> mod(std::shared_ptr<PyObject> other) override;
    
    std::shared_ptr<PyObject> eq(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> ne(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> lt(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> le(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> gt(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> ge(std::shared_ptr<PyObject> other) override;
};

// Float type
class PyFloat : public PyObject {
private:
    double value_;
    
public:
    explicit PyFloat(double value) : PyObject(PyType::FLOAT), value_(value) {}
    
    double value() const { return value_; }
    std::string to_string() const override { return std::to_string(value_); }
    bool is_true() const override { return value_ != 0.0; }
    
    std::shared_ptr<PyObject> add(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> sub(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> mul(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> div(std::shared_ptr<PyObject> other) override;
    
    std::shared_ptr<PyObject> eq(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> lt(std::shared_ptr<PyObject> other) override;
};

// String type
class PyString : public PyObject {
private:
    std::string value_;
    
public:
    explicit PyString(const std::string& value) 
        : PyObject(PyType::STRING), value_(value) {}
    
    const std::string& value() const { return value_; }
    std::string to_string() const override { return value_; }
    bool is_true() const override { return !value_.empty(); }
    size_t len() const override { return value_.size(); }
    
    std::shared_ptr<PyObject> add(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> mul(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> eq(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> getitem(std::shared_ptr<PyObject> key) override;
};

// List type
class PyList : public PyObject {
private:
    std::vector<std::shared_ptr<PyObject>> items_;
    
public:
    PyList() : PyObject(PyType::LIST) {}
    explicit PyList(const std::vector<std::shared_ptr<PyObject>>& items)
        : PyObject(PyType::LIST), items_(items) {}
    
    const std::vector<std::shared_ptr<PyObject>>& items() const { return items_; }
    std::vector<std::shared_ptr<PyObject>>& items() { return items_; }
    
    std::string to_string() const override;
    bool is_true() const override { return !items_.empty(); }
    size_t len() const override { return items_.size(); }
    
    void append(std::shared_ptr<PyObject> item) { items_.push_back(item); }
    void insert(size_t index, std::shared_ptr<PyObject> item);
    void remove(size_t index);
    
    std::shared_ptr<PyObject> getitem(std::shared_ptr<PyObject> key) override;
    void setitem(std::shared_ptr<PyObject> key, std::shared_ptr<PyObject> value) override;
    
    std::shared_ptr<PyObject> add(std::shared_ptr<PyObject> other) override;
    std::shared_ptr<PyObject> mul(std::shared_ptr<PyObject> other) override;
};

// Dictionary type
class PyDict : public PyObject {
private:
    std::unordered_map<std::string, std::shared_ptr<PyObject>> items_;
    
public:
    PyDict() : PyObject(PyType::DICT) {}
    
    const auto& items() const { return items_; }
    auto& items() { return items_; }
    
    std::string to_string() const override;
    bool is_true() const override { return !items_.empty(); }
    size_t len() const override { return items_.size(); }
    
    std::shared_ptr<PyObject> getitem(std::shared_ptr<PyObject> key) override;
    void setitem(std::shared_ptr<PyObject> key, std::shared_ptr<PyObject> value) override;
    
    bool contains(const std::string& key) const {
        return items_.find(key) != items_.end();
    }
};

// Function type
class PyFunction : public PyObject {
public:
    using FunctionPtr = std::function<std::shared_ptr<PyObject>(const std::vector<std::shared_ptr<PyObject>>&)>;
    
private:
    std::string name_;
    FunctionPtr func_;
    std::vector<std::string> param_names_;
    std::shared_ptr<PyDict> closure_;
    
public:
    PyFunction(const std::string& name, FunctionPtr func)
        : PyObject(PyType::FUNCTION), name_(name), func_(func), 
          closure_(std::make_shared<PyDict>()) {}
    
    const std::string& name() const { return name_; }
    std::string to_string() const override { return "<function " + name_ + ">"; }
    
    std::shared_ptr<PyObject> call(const std::vector<std::shared_ptr<PyObject>>& args) override {
        return func_(args);
    }
    
    void set_closure(std::shared_ptr<PyDict> closure) { closure_ = closure; }
    std::shared_ptr<PyDict> closure() { return closure_; }
};

// Exception type
class PyException : public PyObject {
private:
    std::string message_;
    std::string type_name_;
    std::vector<std::string> traceback_;
    
public:
    PyException(const std::string& type_name, const std::string& message)
        : PyObject(PyType::EXCEPTION), type_name_(type_name), message_(message) {}
    
    const std::string& message() const { return message_; }
    const std::string& type_name() const { return type_name_; }
    const std::vector<std::string>& traceback() const { return traceback_; }
    
    void add_traceback(const std::string& frame) {
        traceback_.push_back(frame);
    }
    
    std::string to_string() const override {
        return type_name_ + ": " + message_;
    }
};

// Class type
class PyClass : public PyObject {
private:
    std::string name_;
    std::vector<std::shared_ptr<PyClass>> bases_;
    std::unordered_map<std::string, std::shared_ptr<PyFunction>> methods_;
    
public:
    explicit PyClass(const std::string& name)
        : PyObject(PyType::CLASS), name_(name) {}
    
    const std::string& name() const { return name_; }
    void add_base(std::shared_ptr<PyClass> base) { bases_.push_back(base); }
    void add_method(const std::string& name, std::shared_ptr<PyFunction> method) {
        methods_[name] = method;
    }
    
    std::shared_ptr<PyFunction> get_method(const std::string& name);
    
    std::string to_string() const override {
        return "<class '" + name_ + "'>";
    }
    
    // Create instance
    std::shared_ptr<PyObject> call(const std::vector<std::shared_ptr<PyObject>>& args) override;
};

// Instance type
class PyInstance : public PyObject {
private:
    std::shared_ptr<PyClass> class_;
    
public:
    explicit PyInstance(std::shared_ptr<PyClass> cls)
        : PyObject(PyType::INSTANCE), class_(cls) {}
    
    std::shared_ptr<PyClass> py_class() { return class_; }
    
    std::string to_string() const override {
        return "<instance of " + class_->name() + ">";
    }
    
    std::shared_ptr<PyObject> get_attr(const std::string& name);
};

// Generator type
class PyGenerator : public PyObject {
public:
    enum class State {
        CREATED,
        RUNNING,
        SUSPENDED,
        COMPLETED
    };
    
private:
    State state_;
    void* continuation_;  // Continuation for resuming
    std::shared_ptr<PyObject> current_value_;
    
public:
    PyGenerator() 
        : PyObject(PyType::GENERATOR), 
          state_(State::CREATED),
          continuation_(nullptr) {}
    
    State state() const { return state_; }
    void set_state(State state) { state_ = state; }
    
    std::shared_ptr<PyObject> next();
    void send(std::shared_ptr<PyObject> value);
    
    std::string to_string() const override { return "<generator>"; }
};

// Helper functions
std::shared_ptr<PyObject> make_int(int64_t value);
std::shared_ptr<PyObject> make_float(double value);
std::shared_ptr<PyObject> make_string(const std::string& value);
std::shared_ptr<PyObject> make_bool(bool value);
std::shared_ptr<PyObject> make_list(const std::vector<std::shared_ptr<PyObject>>& items = {});
std::shared_ptr<PyObject> make_dict();
std::shared_ptr<PyObject> make_none();

} // namespace pyvm::runtime