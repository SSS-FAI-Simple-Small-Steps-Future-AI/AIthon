#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>

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

// Base Python object - all Python values derive from this
class PyObject {
protected:
    PyType type_;
    size_t ref_count_;
    bool marked_; // For GC

public:
    explicit PyObject(PyType type)
        : type_(type), ref_count_(1), marked_(false) {}

    virtual ~PyObject() = default;

    PyType type() const { return type_; }

    // Reference counting
    void incref() { ref_count_++; }
    void decref() {
        if (--ref_count_ == 0) {
            delete this;
        }
    }
    size_t refcount() const { return ref_count_; }

    // GC marking
    void mark() { marked_ = true; }
    void unmark() { marked_ = false; }
    bool is_marked() const { return marked_; }

    // Type checking
    bool is_none() const { return type_ == PyType::NONE; }
    bool is_bool() const { return type_ == PyType::BOOL; }
    bool is_int() const { return type_ == PyType::INT; }
    bool is_float() const { return type_ == PyType::FLOAT; }
    bool is_string() const { return type_ == PyType::STRING; }
    bool is_list() const { return type_ == PyType::LIST; }
    bool is_dict() const { return type_ == PyType::DICT; }
    bool is_tuple() const { return type_ == PyType::TUPLE; }
    bool is_function() const { return type_ == PyType::FUNCTION; }
    bool is_class() const { return type_ == PyType::CLASS; }
    bool is_instance() const { return type_ == PyType::INSTANCE; }

    // Arithmetic operations
    virtual PyObject* add(PyObject* other);
    virtual PyObject* sub(PyObject* other);
    virtual PyObject* mul(PyObject* other);
    virtual PyObject* div(PyObject* other);
    virtual PyObject* mod(PyObject* other);
    virtual PyObject* pow(PyObject* other);

    // Comparison operations
    virtual PyObject* eq(PyObject* other);
    virtual PyObject* ne(PyObject* other);
    virtual PyObject* lt(PyObject* other);
    virtual PyObject* le(PyObject* other);
    virtual PyObject* gt(PyObject* other);
    virtual PyObject* ge(PyObject* other);

    // Unary operations
    virtual PyObject* neg();
    virtual PyObject* pos();
    virtual PyObject* invert();

    // Container operations
    virtual PyObject* get_item(PyObject* key);
    virtual void set_item(PyObject* key, PyObject* value);

    // Callable
    virtual PyObject* call(PyObject** args, size_t nargs);

    // Conversions
    virtual std::string to_string() const;
    virtual bool to_bool() const;
    virtual int64_t hash() const;
};

// None type
class PyNone : public PyObject {
private:
    PyNone() : PyObject(PyType::NONE) {}

public:
    static PyNone* instance();

    std::string to_string() const override;
    bool to_bool() const override;
};

// Boolean type
class PyBool : public PyObject {
private:
    bool value_;

    explicit PyBool(bool value) : PyObject(PyType::BOOL), value_(value) {}

public:
    static PyBool* get(bool value);

    bool value() const { return value_; }

    std::string to_string() const override;
    bool to_bool() const override;

    PyObject* eq(PyObject* other) override;
};

// Integer type
class PyInt : public PyObject {
private:
    int64_t value_;

public:
    explicit PyInt(int64_t value) : PyObject(PyType::INT), value_(value) {}

    int64_t value() const { return value_; }

    PyObject* add(PyObject* other) override;
    PyObject* sub(PyObject* other) override;
    PyObject* mul(PyObject* other) override;
    PyObject* div(PyObject* other) override;
    PyObject* mod(PyObject* other) override;
    PyObject* pow(PyObject* other) override;

    PyObject* eq(PyObject* other) override;
    PyObject* ne(PyObject* other) override;
    PyObject* lt(PyObject* other) override;
    PyObject* le(PyObject* other) override;
    PyObject* gt(PyObject* other) override;
    PyObject* ge(PyObject* other) override;

    PyObject* neg() override;

    std::string to_string() const override;
    bool to_bool() const override;
    int64_t hash() const override;
};

// Float type
class PyFloat : public PyObject {
private:
    double value_;

public:
    explicit PyFloat(double value) : PyObject(PyType::FLOAT), value_(value) {}

    double value() const { return value_; }

    PyObject* add(PyObject* other) override;
    PyObject* sub(PyObject* other) override;
    PyObject* mul(PyObject* other) override;
    PyObject* div(PyObject* other) override;

    PyObject* eq(PyObject* other) override;
    PyObject* lt(PyObject* other) override;

    PyObject* neg() override;

    std::string to_string() const override;
    bool to_bool() const override;
};

// String type
class PyString : public PyObject {
private:
    std::string value_;

public:
    explicit PyString(const std::string& value)
        : PyObject(PyType::STRING), value_(value) {}

    const std::string& value() const { return value_; }

    PyObject* add(PyObject* other) override;
    PyObject* mul(PyObject* other) override;
    PyObject* eq(PyObject* other) override;
    PyObject* get_item(PyObject* key) override;

    std::string to_string() const override;
    bool to_bool() const override;
    size_t length() const;
    int64_t hash() const override;
};

// List type
class PyList : public PyObject {
private:
    std::vector<PyObject*> items_;

public:
    PyList() : PyObject(PyType::LIST) {}

    ~PyList() {
        for (auto* item : items_) {
            if (item) item->decref();
        }
    }

    void append(PyObject* item);
    PyObject* get(size_t index) const;
    void set(size_t index, PyObject* value);
    size_t size() const { return items_.size(); }

    PyObject* get_item(PyObject* key) override;
    void set_item(PyObject* key, PyObject* value) override;

    std::string to_string() const override;
    bool to_bool() const override;
    size_t length() const;

    // Iterator support
    const std::vector<PyObject*>& items() const { return items_; }
};

// Dictionary type
class PyDict : public PyObject {
private:
    std::unordered_map<std::string, PyObject*> items_;

public:
    PyDict() : PyObject(PyType::DICT) {}

    ~PyDict() {
        for (auto& [key, value] : items_) {
            if (value) value->decref();
        }
    }

    void set(const std::string& key, PyObject* value);
    PyObject* get(const std::string& key) const;
    bool contains(const std::string& key) const {
        return items_.find(key) != items_.end();
    }

    PyObject* get_item(PyObject* key) override;
    void set_item(PyObject* key, PyObject* value) override;

    std::string to_string() const override;
    bool to_bool() const override;
    size_t length() const;

    // Iterator support
    const std::unordered_map<std::string, PyObject*>& items() const { return items_; }
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

    // std::shared_ptr<PyObject> call(const std::vector<std::shared_ptr<PyObject>>& args) override {
    //     return func_(args);
    // }
    std::shared_ptr<PyObject> call(const std::vector<std::shared_ptr<PyObject>>& args);


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
    std::shared_ptr<PyObject> call(const std::vector<std::shared_ptr<PyObject>>& args);
    // std::shared_ptr<PyObject> call(const std::vector<std::shared_ptr<PyObject>>& args) override;

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


}