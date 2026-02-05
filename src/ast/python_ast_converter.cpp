#include "ast/python_ast_converter.h"
#include <stdexcept>
#include <iostream>

namespace pyvm::ast {

PythonASTConverter::PythonASTConverter() {
    // Initialize Python interpreter
    Py_Initialize();
    
    // Import ast module
    py_ast_module_ = PyImport_ImportModule("ast");
    if (!py_ast_module_) {
        PyErr_Print();
        throw std::runtime_error("Failed to import Python ast module");
    }
    
    // Get compile function
    py_compile_func_ = PyObject_GetAttrString(py_ast_module_, "parse");
    if (!py_compile_func_) {
        PyErr_Print();
        throw std::runtime_error("Failed to get ast.parse function");
    }
}

PythonASTConverter::~PythonASTConverter() {
    Py_XDECREF(py_compile_func_);
    Py_XDECREF(py_ast_module_);
    Py_Finalize();
}

std::unique_ptr<Module> PythonASTConverter::parse_file(const std::string& filename) {
    // Read file
    FILE* fp = fopen(filename.c_str(), "r");
    if (!fp) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // Read content
    std::string content(size, '\0');
    fread(&content[0], 1, size, fp);
    fclose(fp);
    
    return parse_string(content);
}

std::unique_ptr<Module> PythonASTConverter::parse_string(const std::string& source) {
    // Parse Python source
    PyObject* py_source = PyUnicode_FromString(source.c_str());
    PyObject* args = PyTuple_Pack(1, py_source);
    PyObject* py_ast = PyObject_CallObject(py_compile_func_, args);
    
    Py_DECREF(args);
    Py_DECREF(py_source);
    
    if (!py_ast) {
        PyErr_Print();
        throw std::runtime_error("Failed to parse Python source");
    }
    
    // Convert to our AST
    auto module = convert_module(py_ast);
    
    Py_DECREF(py_ast);
    
    return module;
}

std::unique_ptr<Module> PythonASTConverter::convert_module(PyObject* py_module) {
    auto module = std::make_unique<Module>();
    
    PyObject* body = get_attr(py_module, "body");
    if (body && PyList_Check(body)) {
        module->body = convert_node_list(body);
    }
    
    return module;
}

ASTNodePtr PythonASTConverter::convert_node(PyObject* py_node) {
    if (!py_node) return nullptr;
    
    // Get node class name
    PyObject* class_obj = PyObject_GetAttrString(py_node, "__class__");
    PyObject* class_name = PyObject_GetAttrString(class_obj, "__name__");
    std::string name = PyUnicode_AsUTF8(class_name);
    Py_DECREF(class_name);
    Py_DECREF(class_obj);
    
    // Convert based on type
    if (name == "FunctionDef" || name == "AsyncFunctionDef") {
        return convert_function_def(py_node);
    } else if (name == "Return") {
        return convert_return(py_node);
    } else if (name == "Assign") {
        return convert_assign(py_node);
    } else if (name == "Expr") {
        return convert_expr_stmt(py_node);
    } else if (name == "BinOp") {
        return convert_binop(py_node);
    } else if (name == "UnaryOp") {
        return convert_unaryop(py_node);
    } else if (name == "Compare") {
        return convert_compare(py_node);
    } else if (name == "Call") {
        return convert_call(py_node);
    } else if (name == "Await") {
        return convert_await(py_node);
    } else if (name == "Name") {
        return convert_name(py_node);
    } else if (name == "Constant") {
        return convert_constant(py_node);
    } else if (name == "If") {
        return convert_if(py_node);
    } else if (name == "While") {
        return convert_while(py_node);
    } else if (name == "For") {
        return convert_for(py_node);
    } else if (name == "Pass") {
        return std::make_unique<Pass>();
    } else if (name == "Break") {
        return std::make_unique<Break>();
    } else if (name == "Continue") {
        return std::make_unique<Continue>();
    }
    
    std::cerr << "Warning: Unsupported node type: " << name << std::endl;
    return nullptr;
}

std::unique_ptr<FunctionDef> PythonASTConverter::convert_function_def(PyObject* py_func) {
    std::string name = get_string_attr(py_func, "name");
    
    // Check if async
    PyObject* class_obj = PyObject_GetAttrString(py_func, "__class__");
    PyObject* class_name = PyObject_GetAttrString(class_obj, "__name__");
    bool is_async = (std::string(PyUnicode_AsUTF8(class_name)) == "AsyncFunctionDef");
    Py_DECREF(class_name);
    Py_DECREF(class_obj);
    
    auto func = std::make_unique<FunctionDef>(name, is_async);
    
    // Get arguments
    PyObject* args_obj = get_attr(py_func, "args");
    if (args_obj) {
        PyObject* args_list = get_attr(args_obj, "args");
        if (args_list && PyList_Check(args_list)) {
            Py_ssize_t size = PyList_Size(args_list);
            for (Py_ssize_t i = 0; i < size; i++) {
                PyObject* arg = PyList_GetItem(args_list, i);
                std::string arg_name = get_string_attr(arg, "arg");
                func->args.push_back(arg_name);
            }
        }
    }
    
    // Get body
    PyObject* body = get_attr(py_func, "body");
    if (body && PyList_Check(body)) {
        func->body = convert_node_list(body);
    }
    
    return func;
}

std::unique_ptr<Return> PythonASTConverter::convert_return(PyObject* py_return) {
    auto ret = std::make_unique<Return>();
    
    PyObject* value = get_attr(py_return, "value");
    if (value && value != Py_None) {
        ret->value = convert_node(value);
    }
    
    return ret;
}

std::unique_ptr<Assign> PythonASTConverter::convert_assign(PyObject* py_assign) {
    auto assign = std::make_unique<Assign>();
    
    // Get targets
    PyObject* targets = get_attr(py_assign, "targets");
    if (targets && PyList_Check(targets)) {
        assign->targets = convert_node_list(targets);
    }
    
    // Get value
    PyObject* value = get_attr(py_assign, "value");
    if (value) {
        assign->value = convert_node(value);
    }
    
    return assign;
}

std::unique_ptr<Expr> PythonASTConverter::convert_expr_stmt(PyObject* py_expr) {
    auto expr = std::make_unique<Expr>();
    
    PyObject* value = get_attr(py_expr, "value");
    if (value) {
        expr->value = convert_node(value);
    }
    
    return expr;
}

std::unique_ptr<BinOp> PythonASTConverter::convert_binop(PyObject* py_binop) {
    auto binop = std::make_unique<BinOp>();
    
    PyObject* left = get_attr(py_binop, "left");
    if (left) {
        binop->left = convert_node(left);
    }
    
    PyObject* op = get_attr(py_binop, "op");
    if (op) {
        binop->op = convert_binary_operator(op);
    }
    
    PyObject* right = get_attr(py_binop, "right");
    if (right) {
        binop->right = convert_node(right);
    }
    
    return binop;
}

std::unique_ptr<UnaryOp> PythonASTConverter::convert_unaryop(PyObject* py_unaryop) {
    auto unaryop = std::make_unique<UnaryOp>();
    
    PyObject* op = get_attr(py_unaryop, "op");
    if (op) {
        unaryop->op = convert_unary_operator(op);
    }
    
    PyObject* operand = get_attr(py_unaryop, "operand");
    if (operand) {
        unaryop->operand = convert_node(operand);
    }
    
    return unaryop;
}

std::unique_ptr<Compare> PythonASTConverter::convert_compare(PyObject* py_compare) {
    auto compare = std::make_unique<Compare>();
    
    PyObject* left = get_attr(py_compare, "left");
    if (left) {
        compare->left = convert_node(left);
    }
    
    // Get operators
    PyObject* ops = get_attr(py_compare, "ops");
    if (ops && PyList_Check(ops)) {
        Py_ssize_t size = PyList_Size(ops);
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject* op = PyList_GetItem(ops, i);
            compare->ops.push_back(convert_compare_operator(op));
        }
    }
    
    // Get comparators
    PyObject* comparators = get_attr(py_compare, "comparators");
    if (comparators && PyList_Check(comparators)) {
        compare->comparators = convert_node_list(comparators);
    }
    
    return compare;
}

std::unique_ptr<Call> PythonASTConverter::convert_call(PyObject* py_call) {
    auto call = std::make_unique<Call>();
    
    PyObject* func = get_attr(py_call, "func");
    if (func) {
        call->func = convert_node(func);
    }
    
    PyObject* args = get_attr(py_call, "args");
    if (args && PyList_Check(args)) {
        call->args = convert_node_list(args);
    }
    
    return call;
}

std::unique_ptr<Await> PythonASTConverter::convert_await(PyObject* py_await) {
    auto await_expr = std::make_unique<Await>();
    
    PyObject* value = get_attr(py_await, "value");
    if (value) {
        await_expr->value = convert_node(value);
    }
    
    return await_expr;
}

std::unique_ptr<Name> PythonASTConverter::convert_name(PyObject* py_name) {
    std::string id = get_string_attr(py_name, "id");
    return std::make_unique<Name>(id);
}

std::unique_ptr<Constant> PythonASTConverter::convert_constant(PyObject* py_const) {
    PyObject* value = get_attr(py_const, "value");
    
    if (PyLong_Check(value)) {
        int64_t val = PyLong_AsLongLong(value);
        return std::make_unique<Constant>(val);
    } else if (PyFloat_Check(value)) {
        double val = PyFloat_AsDouble(value);
        return std::make_unique<Constant>(val);
    } else if (PyUnicode_Check(value)) {
        std::string val = PyUnicode_AsUTF8(value);
        return std::make_unique<Constant>(val);
    } else if (PyBool_Check(value)) {
        bool val = (value == Py_True);
        return std::make_unique<Constant>(val);
    }
    
    return std::make_unique<Constant>(0L);
}

std::unique_ptr<If> PythonASTConverter::convert_if(PyObject* py_if) {
    auto if_stmt = std::make_unique<If>();
    
    PyObject* test = get_attr(py_if, "test");
    if (test) {
        if_stmt->test = convert_node(test);
    }
    
    PyObject* body = get_attr(py_if, "body");
    if (body && PyList_Check(body)) {
        if_stmt->body = convert_node_list(body);
    }
    
    PyObject* orelse = get_attr(py_if, "orelse");
    if (orelse && PyList_Check(orelse)) {
        if_stmt->orelse = convert_node_list(orelse);
    }
    
    return if_stmt;
}

std::unique_ptr<While> PythonASTConverter::convert_while(PyObject* py_while) {
    auto while_stmt = std::make_unique<While>();
    
    PyObject* test = get_attr(py_while, "test");
    if (test) {
        while_stmt->test = convert_node(test);
    }
    
    PyObject* body = get_attr(py_while, "body");
    if (body && PyList_Check(body)) {
        while_stmt->body = convert_node_list(body);
    }
    
    return while_stmt;
}

std::unique_ptr<For> PythonASTConverter::convert_for(PyObject* py_for) {
    // Check if async
    PyObject* class_obj = PyObject_GetAttrString(py_for, "__class__");
    PyObject* class_name = PyObject_GetAttrString(class_obj, "__name__");
    bool is_async = (std::string(PyUnicode_AsUTF8(class_name)) == "AsyncFor");
    Py_DECREF(class_name);
    Py_DECREF(class_obj);
    
    auto for_stmt = std::make_unique<For>();
    for_stmt->is_async = is_async;
    
    PyObject* target = get_attr(py_for, "target");
    if (target) {
        for_stmt->target = convert_node(target);
    }
    
    PyObject* iter = get_attr(py_for, "iter");
    if (iter) {
        for_stmt->iter = convert_node(iter);
    }
    
    PyObject* body = get_attr(py_for, "body");
    if (body && PyList_Check(body)) {
        for_stmt->body = convert_node_list(body);
    }
    
    return for_stmt;
}

// Helper functions

std::string PythonASTConverter::get_string_attr(PyObject* obj, const char* attr) {
    PyObject* py_str = PyObject_GetAttrString(obj, attr);
    if (!py_str) return "";
    
    std::string result = PyUnicode_AsUTF8(py_str);
    Py_DECREF(py_str);
    return result;
}

PyObject* PythonASTConverter::get_attr(PyObject* obj, const char* attr) {
    return PyObject_GetAttrString(obj, attr);
}

std::vector<ASTNodePtr> PythonASTConverter::convert_node_list(PyObject* py_list) {
    std::vector<ASTNodePtr> result;
    
    if (!PyList_Check(py_list)) return result;
    
    Py_ssize_t size = PyList_Size(py_list);
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject* item = PyList_GetItem(py_list, i);
        auto node = convert_node(item);
        if (node) {
            result.push_back(std::move(node));
        }
    }
    
    return result;
}

BinaryOp PythonASTConverter::convert_binary_operator(PyObject* py_op) {
    PyObject* class_obj = PyObject_GetAttrString(py_op, "__class__");
    PyObject* class_name = PyObject_GetAttrString(class_obj, "__name__");
    std::string name = PyUnicode_AsUTF8(class_name);
    Py_DECREF(class_name);
    Py_DECREF(class_obj);
    
    if (name == "Add") return BinaryOp::ADD;
    if (name == "Sub") return BinaryOp::SUB;
    if (name == "Mult") return BinaryOp::MULT;
    if (name == "Div") return BinaryOp::DIV;
    if (name == "Mod") return BinaryOp::MOD;
    if (name == "Pow") return BinaryOp::POW;
    if (name == "LShift") return BinaryOp::LSHIFT;
    if (name == "RShift") return BinaryOp::RSHIFT;
    if (name == "BitOr") return BinaryOp::BITOR;
    if (name == "BitXor") return BinaryOp::BITXOR;
    if (name == "BitAnd") return BinaryOp::BITAND;
    if (name == "FloorDiv") return BinaryOp::FLOORDIV;
    
    return BinaryOp::ADD;
}

UnaryOp PythonASTConverter::convert_unary_operator(PyObject* py_op) {
    PyObject* class_obj = PyObject_GetAttrString(py_op, "__class__");
    PyObject* class_name = PyObject_GetAttrString(class_obj, "__name__");
    std::string name = PyUnicode_AsUTF8(class_name);
    Py_DECREF(class_name);
    Py_DECREF(class_obj);
    
    if (name == "Invert") return UnaryOp::INVERT;
    if (name == "Not") return UnaryOp::NOT;
    if (name == "UAdd") return UnaryOp::UADD;
    if (name == "USub") return UnaryOp::USUB;
    
    return UnaryOp::NOT;
}

CompareOp PythonASTConverter::convert_compare_operator(PyObject* py_op) {
    PyObject* class_obj = PyObject_GetAttrString(py_op, "__class__");
    PyObject* class_name = PyObject_GetAttrString(class_obj, "__name__");
    std::string name = PyUnicode_AsUTF8(class_name);
    Py_DECREF(class_name);
    Py_DECREF(class_obj);
    
    if (name == "Eq") return CompareOp::EQ;
    if (name == "NotEq") return CompareOp::NOTEQ;
    if (name == "Lt") return CompareOp::LT;
    if (name == "LtE") return CompareOp::LTE;
    if (name == "Gt") return CompareOp::GT;
    if (name == "GtE") return CompareOp::GTE;
    if (name == "Is") return CompareOp::IS;
    if (name == "IsNot") return CompareOp::ISNOT;
    if (name == "In") return CompareOp::IN;
    if (name == "NotIn") return CompareOp::NOTIN;
    
    return CompareOp::EQ;
}

} // namespace pyvm::ast