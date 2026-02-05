#pragma once

#include "ast_nodes.h"
#include <Python.h>
#include <string>

namespace pyvm::ast {

    class PythonASTConverter {
    private:
        PyObject* py_ast_module_;
        PyObject* py_compile_func_;

    public:
        PythonASTConverter();
        ~PythonASTConverter();

        // Parse Python source code and convert to our AST
        std::unique_ptr<Module> parse_file(const std::string& filename);
        std::unique_ptr<Module> parse_string(const std::string& source);

    private:
        // Convert Python AST nodes to our AST
        ASTNodePtr convert_node(PyObject* py_node);
        std::unique_ptr<Module> convert_module(PyObject* py_module);
        std::unique_ptr<FunctionDef> convert_function_def(PyObject* py_func);
        std::unique_ptr<Return> convert_return(PyObject* py_return);
        std::unique_ptr<Assign> convert_assign(PyObject* py_assign);
        std::unique_ptr<Expr> convert_expr_stmt(PyObject* py_expr);
        std::unique_ptr<BinOp> convert_binop(PyObject* py_binop);
        std::unique_ptr<UnaryOp> convert_unaryop(PyObject* py_unaryop);
        std::unique_ptr<Compare> convert_compare(PyObject* py_compare);
        std::unique_ptr<Call> convert_call(PyObject* py_call);
        std::unique_ptr<Await> convert_await(PyObject* py_await);
        std::unique_ptr<Name> convert_name(PyObject* py_name);
        std::unique_ptr<Constant> convert_constant(PyObject* py_const);
        std::unique_ptr<If> convert_if(PyObject* py_if);
        std::unique_ptr<While> convert_while(PyObject* py_while);
        std::unique_ptr<For> convert_for(PyObject* py_for);

        // Helper functions
        std::string get_string_attr(PyObject* obj, const char* attr);
        PyObject* get_attr(PyObject* obj, const char* attr);
        std::vector<ASTNodePtr> convert_node_list(PyObject* py_list);

        BinaryOp convert_binary_operator(PyObject* py_op);
        UnaryOp convert_unary_operator(PyObject* py_op);
        CompareOp convert_compare_operator(PyObject* py_op);
    };

} // namespace pyvm::ast