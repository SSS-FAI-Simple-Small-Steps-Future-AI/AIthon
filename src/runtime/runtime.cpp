
#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstring>
#include <atomic>  // for std::atomic
// ============================================================================
// Runtime Data Structures
// ============================================================================

namespace llvm {
    class Function;
}

// Type tags for runtime type checking
enum class ValueType : uint8_t {
    INT,
    FLOAT,
    STRING,
    BOOL,
    LIST,
    DICT,
    NONE
};

// Generic value wrapper (like Python's PyObject)
struct RuntimeValue {
    ValueType type;
    union {
        int64_t int_val;
        double float_val;
        bool bool_val;
        void* ptr_val;  // For strings, lists, dicts
    } data{};

    RuntimeValue() : type(ValueType::NONE) {
        data.int_val = 0;
    }
};

// List structure (heap-allocated)
struct RuntimeList {
    std::vector<RuntimeValue> items;

    RuntimeList() = default;

    void append(const RuntimeValue& val) {
        items.push_back(val);
    }

    RuntimeValue get(size_t index) {
        if (index >= items.size()) {
            std::cerr << "IndexError: list index out of range\n";
            RuntimeValue err;
            err.type = ValueType::NONE;
            return err;
        }
        return items[index];
    }

    size_t size() const {
        return items.size();
    }
};

// Dictionary structure (heap-allocated)
struct RuntimeDict {
    std::unordered_map<std::string, RuntimeValue> items;

    RuntimeDict() = default;

    void set(const std::string& key, const RuntimeValue& val) {
        items[key] = val;
    }

    RuntimeValue get(const std::string& key) {
        auto it = items.find(key);
        if (it == items.end()) {
            std::cerr << "KeyError: '" << key << "'\n";
            RuntimeValue err;
            err.type = ValueType::NONE;
            return err;
        }
        return it->second;
    }

    bool has_key(const std::string& key) const {
        return items.find(key) != items.end();
    }
};

// ============================================================================
// Runtime Functions (called from LLVM)
// ============================================================================

extern "C" {

// --- Print Functions ---

void runtime_print_int(int64_t value) {
    std::cout << value << std::endl;
}

void runtime_print_float(double value) {
    std::cout << value << std::endl;
}

void runtime_print_string(const char* str) {
    if (str) {
        std::cout << str << std::endl;
    }
}

void runtime_print_bool(bool value) {
    std::cout << (value ? "True" : "False") << std::endl;
}

void runtime_print_value(RuntimeValue* val) {
    if (!val) {
        std::cout << "None" << std::endl;
        return;
    }

    switch (val->type) {
        case ValueType::INT:
            std::cout << val->data.int_val << std::endl;
            break;
        case ValueType::FLOAT:
            std::cout << val->data.float_val << std::endl;
            break;
        case ValueType::STRING:
            if (val->data.ptr_val) {
                std::cout << static_cast<char*>(val->data.ptr_val) << std::endl;
            }
            break;
        case ValueType::BOOL:
            std::cout << (val->data.bool_val ? "True" : "False") << std::endl;
            break;
        case ValueType::LIST:
            std::cout << "[list object]" << std::endl;
            break;
        case ValueType::DICT:
            std::cout << "{dict object}" << std::endl;
            break;
        case ValueType::NONE:
            std::cout << "None" << std::endl;
            break;
    }
}

// --- List Functions ---

// Create a new list on the heap
void* runtime_list_create() {
    return new RuntimeList();
}

// Append an integer to list
void runtime_list_append_int(void* list_ptr, int64_t value) {
    if (!list_ptr) return;

    auto* list = static_cast<RuntimeList*>(list_ptr);
    RuntimeValue val;
    val.type = ValueType::INT;
    val.data.int_val = value;
    list->append(val);
}

// Append a string to list
void runtime_list_append_string(void* list_ptr, const char* str) {
    if (!list_ptr) return;

    auto* list = static_cast<RuntimeList*>(list_ptr);
    RuntimeValue val;
    val.type = ValueType::STRING;

    // Duplicate string
    size_t len = strlen(str);
    char* str_copy = new char[len + 1];
    strcpy(str_copy, str);

    val.data.ptr_val = str_copy;
    list->append(val);
}

// Get item from list (returns string for now - simplified)
const char* runtime_list_get_string(void* list_ptr, const int64_t index) {
    if (!list_ptr) return nullptr;

    auto* list = static_cast<RuntimeList*>(list_ptr);
    RuntimeValue val = list->get(static_cast<size_t>(index));

    if (val.type == ValueType::STRING && val.data.ptr_val) {
        return static_cast<const char*>(val.data.ptr_val);
    }

    return nullptr;
}

// Get integer from list
int64_t runtime_list_get_int(void* list_ptr, const int64_t index) {
    if (!list_ptr) return 0;

    auto* list = static_cast<RuntimeList*>(list_ptr);
    RuntimeValue val = list->get(static_cast<size_t>(index));

    if (val.type == ValueType::INT) {
        return val.data.int_val;
    }

    return 0;
}

// Get list size
int64_t runtime_list_size(void* list_ptr) {
    if (!list_ptr) return 0;
    const auto* list = static_cast<RuntimeList*>(list_ptr);
    return static_cast<int64_t>(list->size());
}

// Alias for len() builtin
int64_t runtime_list_len(void* list_ptr) {
    return runtime_list_size(list_ptr);
}

// Free list
void runtime_list_free(void* list_ptr) {
    if (!list_ptr) return;

    auto* list = static_cast<RuntimeList*>(list_ptr);

    // Free string data
    for (const auto& item : list->items) {
        if (item.type == ValueType::STRING && item.data.ptr_val) {
            delete[] static_cast<char*>(item.data.ptr_val);
        }
    }

    delete list;
}

// --- Dictionary Functions ---

// Create a new dictionary on the heap
void* runtime_dict_create() {
    return new RuntimeDict();
}

// Set string value in dict
void runtime_dict_set_string(void* dict_ptr, const char* key, const char* value) {
    if (!dict_ptr || !key || !value) return;

    auto* dict = static_cast<RuntimeDict*>(dict_ptr);
    RuntimeValue val;
    val.type = ValueType::STRING;

    // Duplicate string
    size_t len = strlen(value);
    char* str_copy = new char[len + 1];
    strcpy(str_copy, value);

    val.data.ptr_val = str_copy;
    dict->set(std::string(key), val);
}

// Set integer value in dict
void runtime_dict_set_int(void* dict_ptr, const char* key, int64_t value) {
    if (!dict_ptr || !key) return;

    auto* dict = static_cast<RuntimeDict*>(dict_ptr);
    RuntimeValue val;
    val.type = ValueType::INT;
    val.data.int_val = value;
    dict->set(std::string(key), val);
}

// Get string from dict
const char* runtime_dict_get_string(void* dict_ptr, const char* key) {
    if (!dict_ptr || !key) return nullptr;

    auto* dict = static_cast<RuntimeDict*>(dict_ptr);
    RuntimeValue val = dict->get(std::string(key));

    if (val.type == ValueType::STRING && val.data.ptr_val) {
        return static_cast<const char*>(val.data.ptr_val);
    }

    return nullptr;
}

// Get integer from dict
int64_t runtime_dict_get_int(void* dict_ptr, const char* key) {
    if (!dict_ptr || !key) return 0;

    auto* dict = static_cast<RuntimeDict*>(dict_ptr);
    RuntimeValue val = dict->get(std::string(key));

    if (val.type == ValueType::INT) {
        return val.data.int_val;
    }

    return 0;
}

// Check if key exists in dict
bool runtime_dict_has_key(void* dict_ptr, const char* key) {
    if (!dict_ptr || !key) return false;

    auto* dict = static_cast<RuntimeDict*>(dict_ptr);
    return dict->has_key(std::string(key));
}

// Free dictionary
void runtime_dict_free(void* dict_ptr) {
    if (!dict_ptr) return;

    auto* dict = static_cast<RuntimeDict*>(dict_ptr);

    // Free string data
    for (auto& pair : dict->items) {
        if (pair.second.type == ValueType::STRING && pair.second.data.ptr_val) {
            delete[] static_cast<char*>(pair.second.data.ptr_val);
        }
    }

    delete dict;
}


// ============================================================================
// Print List (for list of strings)
// ============================================================================
void runtime_list_print(void* list_ptr) {
    if (!list_ptr) {
        std::cout << "[]" << std::endl;
        return;
    }

    const auto* list = static_cast<RuntimeList*>(list_ptr);

    std::cout << "[";
    for (size_t i = 0; i < list->items.size(); i++) {
        if (i > 0) std::cout << ", ";

        auto& item = list->items[i];

        // Print based on type
        switch (item.type) {
            case ValueType::INT:
                std::cout << item.data.int_val;
                break;
            case ValueType::FLOAT:
                std::cout << item.data.float_val;
                break;
            case ValueType::STRING:
                if (item.data.ptr_val) {
                    std::cout << "\"" << static_cast<char*>(item.data.ptr_val) << "\"";
                }
                break;
            case ValueType::BOOL:
                std::cout << (item.data.bool_val ? "True" : "False");
                break;
            case ValueType::NONE:
                std::cout << "None";
                break;
            default:
                std::cout << "<object>";
                break;
        }
    }
    std::cout << "]" << std::endl;
}

// ============================================================================
// Print Dictionary (for dict with string keys)
// ============================================================================
void runtime_dict_print(void* dict_ptr) {
    if (!dict_ptr) {
        std::cout << "{}" << std::endl;
        return;
    }

    auto* dict = static_cast<RuntimeDict*>(dict_ptr);

    std::cout << "{";
    bool first = true;
    for (const auto& [key, value] : dict->items) {
        if (!first) std::cout << ", ";
        first = false;

        std::cout << "\"" << key << "\": ";

        // Print value based on type
        switch (value.type) {
            case ValueType::INT:
                std::cout << value.data.int_val;
                break;
            case ValueType::FLOAT:
                std::cout << value.data.float_val;
                break;
            case ValueType::STRING:
                if (value.data.ptr_val) {
                    std::cout << "\"" << static_cast<char*>(value.data.ptr_val) << "\"";
                }
                break;
            case ValueType::BOOL:
                std::cout << (value.data.bool_val ? "True" : "False");
                break;
            case ValueType::NONE:
                std::cout << "None";
                break;
            default:
                std::cout << "<object>";
                break;
        }
    }
    std::cout << "}" << std::endl;
}

/*
class TypeInferenceEngine {
    public:
    aithon::parser::ast::Type infer_field_type(aithon::parser::ast::FieldDecl* field) {
        // Explicit type wins
        if (field->type_annotation) {
            return parse_type(*field->type_annotation);
        }

        // Infer from default value
        if (field->default_value) {
            return infer_from_expr(field->default_value.get());
        }

        // Optional types don't need defaults
        if (is_option_type(field->type_annotation)) {
            return parse_type(*field->type_annotation);
        }

        error("Field must have type or default value");
    }

    private:
    Type infer_from_expr(Expr* expr) {
        if (auto* lit = dynamic_cast<IntegerLiteral*>(expr)) {
            return Type{TypeKind::INT};
        }
        if (auto* lit = dynamic_cast<FloatLiteral*>(expr)) {
            return Type{TypeKind::FLOAT};
        }
        if (auto* lit = dynamic_cast<BoolLiteral*>(expr)) {
            return Type{TypeKind::BOOL};
        }
        if (auto* lit = dynamic_cast<StringLiteral*>(expr)) {
            return Type{TypeKind::STRING};
        }
        if (auto* init = dynamic_cast<InitializerCall*>(expr)) {
            return Type{TypeKind::STRUCT, init->type_name};
        }

        return Type{TypeKind::UNKNOWN};
    }
};
*/

    /*
// Memberwise Initializer
class MemberwiseInitializer {
public:
    // For struct: generate stack-allocated initializer
    llvm::Function* generate_struct_init(aithon::parser::ast::StructDecl* decl) {
        std::string func_name = "init_" + decl->name;

        // Function takes all fields as parameters
        std::vector<llvm::Type*> param_types;
        for (auto& field : decl->fields) {
            param_types.push_back(get_llvm_type(field.resolved_type));
        }

        llvm::FunctionType* func_type = llvm::FunctionType::get(
            struct_types_[decl->name],  // Returns struct by value
            param_types,
            false
        );

        llvm::Function* func = llvm::Function::Create(
            func_type,
            llvm::Function::InternalLinkage,
            func_name,
            module_.get()
        );

        // Generate body
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", func);
        builder_->SetInsertPoint(entry);

        // Allocate struct on stack
        llvm::AllocaInst* struct_ptr = builder_->CreateAlloca(
            struct_types_[decl->name]
        );

        // Set each field from parameters
        size_t idx = 0;
        for (auto& arg : func->args()) {
            llvm::Value* field_ptr = builder_->CreateStructGEP(
                struct_types_[decl->name], struct_ptr, idx
            );
            builder_->CreateStore(&arg, field_ptr);
            idx++;
        }

        // Return struct by value
        llvm::Value* result = builder_->CreateLoad(
            struct_types_[decl->name], struct_ptr
        );
        builder_->CreateRet(result);

        return func;
    }

    // For class: generate heap-allocated initializer
    llvm::Function* generate_class_init(aithon::parser::ast::ClassDecl* decl) {
        std::string func_name = "init_" + decl->name;

        // Function takes all fields as parameters
        std::vector<llvm::Type*> param_types;
        for (auto& field : decl->fields) {
            param_types.push_back(get_llvm_type(field.resolved_type));
        }

        llvm::FunctionType* func_type = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context_),  // Returns HeapObject*
            param_types,
            false
        );

        llvm::Function* func = llvm::Function::Create(
            func_type,
            llvm::Function::ExternalLinkage,
            func_name,
            module_.get()
        );

        // Generate body
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", func);
        builder_->SetInsertPoint(entry);

        // Create class instance on heap
        llvm::Function* create_fn = module_->getFunction("runtime_class_create");
        llvm::Value* class_name = builder_->CreateGlobalStringPtr(decl->name);
        llvm::Value* num_fields = llvm::ConstantInt::get(
            *context_, llvm::APInt(64, decl->fields.size())
        );

        llvm::Value* obj = builder_->CreateCall(create_fn, {class_name, num_fields});

        // Set each field
        size_t idx = 0;
        for (auto& arg : func->args()) {
            llvm::Value* field_idx = llvm::ConstantInt::get(
                *context_, llvm::APInt(64, idx)
            );

            // Call appropriate setter based on type
            llvm::Function* set_fn;
            if (arg.getType()->isIntegerTy(64)) {
                set_fn = module_->getFunction("runtime_class_set_field_int");
            } else if (arg.getType()->isDoubleTy()) {
                set_fn = module_->getFunction("runtime_class_set_field_float");
            } else {
                set_fn = module_->getFunction("runtime_class_set_field_object");
            }

            builder_->CreateCall(set_fn, {obj, field_idx, &arg});
            idx++;
        }

        builder_->CreateRet(obj);

        return func;
    }
};

*/

// ============================================================================
// HeapObject — base structure for all heap-allocated class objects
// ============================================================================

struct HeapObject {
    std::atomic<int64_t> ref_count;  // atomic for thread safety
    const char*          class_name;
    int64_t              num_fields;
    void*                fields[];   // flexible array member (C99)
};

// ============================================================================
// runtime_class_create — allocate class object on heap
// ============================================================================

void* runtime_class_create(const char* class_name, int64_t num_fields) {
    // Allocate: sizeof(HeapObject) + space for fields
    size_t size = sizeof(HeapObject) + (num_fields * sizeof(void*));
    auto* obj = static_cast<HeapObject*>(std::malloc(size));

    if (!obj) {
        std::cerr << "ERROR: Failed to allocate class object: " << class_name << "\n";
        std::exit(1);
    }

    // Initialize
    obj->ref_count.store(1);  // starts with ref_count=1
    obj->class_name = class_name;
    obj->num_fields = num_fields;

    // Zero-initialize all fields
    std::memset(obj->fields, 0, num_fields * sizeof(void*));

    return obj;
}

// ============================================================================
// runtime_retain — increment reference count
// ============================================================================

void* runtime_retain(void* ptr) {
    if (!ptr) return nullptr;

    auto* obj = static_cast<HeapObject*>(ptr);
    obj->ref_count.fetch_add(1, std::memory_order_relaxed);

    return ptr;  // return same pointer
}

// ============================================================================
// runtime_release — decrement reference count, free if reaches zero
// ============================================================================

void runtime_release(void* ptr) {
    if (!ptr) return;

    auto* obj = static_cast<HeapObject*>(ptr);
    int64_t old_count = obj->ref_count.fetch_sub(1, std::memory_order_acq_rel);

    if (old_count == 1) {
        // ref_count reached 0 → deallocate

        // TODO: Release fields that are also heap objects
        // For now, simple free
        std::free(obj);
    }
}

// ============================================================================
// Set field functions — store values by index
// ============================================================================

void runtime_class_set_field_int(void* ptr, int64_t field_idx, int64_t value) {
    if (!ptr) return;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return;

    // Store int as void* (type-punning)
    obj->fields[field_idx] = reinterpret_cast<void*>(value);
}

void runtime_class_set_field_float(void* ptr, int64_t field_idx, double value) {
    if (!ptr) return;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return;

    // Store double as void* (type-punning via union)
    union { double d; void* p; } u{};
    u.d = value;
    obj->fields[field_idx] = u.p;
}

void runtime_class_set_field_bool(void* ptr, int64_t field_idx, bool value) {
    if (!ptr) return;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return;

    obj->fields[field_idx] = reinterpret_cast<void*>(static_cast<uintptr_t>(value));
}

void runtime_class_set_field_ptr(void* ptr, int64_t field_idx, void* value) {
    if (!ptr) return;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return;

    // If the new value is also a heap object, retain it
    // (Assumes all ptr fields are heap objects — refine later)
    if (value) {
        runtime_retain(value);
    }

    // Release old value if it was a heap object
    void* old_value = obj->fields[field_idx];
    if (old_value) {
        runtime_release(old_value);
    }

    obj->fields[field_idx] = value;
}

// ============================================================================
// Get field functions — retrieve values by index
// ============================================================================

int64_t runtime_class_get_field_int(void* ptr, int64_t field_idx) {
    if (!ptr) return 0;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return 0;

    return reinterpret_cast<int64_t>(obj->fields[field_idx]);
}

double runtime_class_get_field_float(void* ptr, int64_t field_idx) {
    if (!ptr) return 0.0;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return 0.0;

    union { void* p; double d; } u{};
    u.p = obj->fields[field_idx];
    return u.d;
}

bool runtime_class_get_field_bool(void* ptr, int64_t field_idx) {
    if (!ptr) return false;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return false;

    return static_cast<bool>(reinterpret_cast<uintptr_t>(obj->fields[field_idx]));
}

void* runtime_class_get_field_ptr(void* ptr, int64_t field_idx) {
    if (!ptr) return nullptr;
    auto* obj = static_cast<HeapObject*>(ptr);
    if (field_idx < 0 || field_idx >= obj->num_fields) return nullptr;

    return obj->fields[field_idx];
}



}