#pragma once

#include <memory>
#include <vector>
#include <string>
#include <llvm/IR/Type.h>


namespace aithon::parser::ast {
    // struct NamedArg;
    // Forward declarations
    class Expr;
    class Stmt;

    // ============================================================================
    // Expressions
    // ============================================================================

    class Expr {
    public:
        virtual ~Expr() = default;
    };

    class IntegerLiteral : public Expr {
    public:
        long long value;
        explicit IntegerLiteral(long long v);
    };

    class FloatLiteral : public Expr {
    public:
        double value;
        explicit FloatLiteral(double v);
    };

    class StringLiteral : public Expr {
    public:
        std::string value;
        explicit StringLiteral(std::string v);
    };

    class BoolLiteral : public Expr {
    public:
        bool value;
        explicit BoolLiteral(bool v);
    };

    class NoneLiteral : public Expr {};

    class Identifier : public Expr {
    public:
        std::string name;
        explicit Identifier(std::string n);
    };

    class BinaryOp : public Expr {
    public:
        enum class Op {
            ADD, SUB, MUL, DIV, MOD, FLOOR_DIV, POW,
            EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
            AND, OR
        };

        Op op;
        std::unique_ptr<Expr> left;
        std::unique_ptr<Expr> right;

        BinaryOp(Op o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r);
    };

    class UnaryOp : public Expr {
    public:
        enum class Op { NEG, NOT };

        Op op;
        std::unique_ptr<Expr> operand;

        UnaryOp(Op o, std::unique_ptr<Expr> operand);
    };

    class CallExpr : public Expr {
    public:
        std::unique_ptr<Expr> callee;
        std::vector<std::unique_ptr<Expr>> arguments;

        CallExpr(std::unique_ptr<Expr> c, std::vector<std::unique_ptr<Expr>> args);
    };

    class IndexExpr : public Expr {
    public:
        std::unique_ptr<Expr> object;
        std::unique_ptr<Expr> index;

        IndexExpr(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx);
    };

    class MemberExpr : public Expr {
    public:
        std::unique_ptr<Expr> object;
        std::string member;

        MemberExpr(std::unique_ptr<Expr> obj, std::string mem);
    };

    class ListExpr : public Expr {
    public:
        std::vector<std::unique_ptr<Expr>> elements;
        explicit ListExpr(std::vector<std::unique_ptr<Expr>> elems);
    };

    class DictExpr : public Expr {
    public:
        std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> pairs;
        explicit DictExpr(std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> p);
    };

    // ============================================================================
    // Statements
    // ============================================================================

    class Stmt {
    public:
        virtual ~Stmt() = default;
    };

    class ExprStmt : public Stmt {
    public:
        std::unique_ptr<Expr> expression;
        explicit ExprStmt(std::unique_ptr<Expr> expr);
    };

    class Assignment : public Stmt {
    public:
        std::string name;
        std::unique_ptr<Expr> value;

        Assignment(std::string n, std::unique_ptr<Expr> v);
    };

    class Block : public Stmt {
    public:
        std::vector<std::unique_ptr<Stmt>> statements;
        explicit Block(std::vector<std::unique_ptr<Stmt>> stmts);
    };

    class IfStmt : public Stmt {
    public:
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Block> then_block;
        std::unique_ptr<Block> else_block;

        IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Block> then_b,
               std::unique_ptr<Block> else_b = nullptr);
    };

    class WhileStmt : public Stmt {
    public:
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Block> body;

        WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Block> b);
    };

    class ForStmt : public Stmt {
    public:
        std::string variable;
        std::unique_ptr<Expr> iterable;
        std::unique_ptr<Block> body;

        ForStmt(std::string var, std::unique_ptr<Expr> iter, std::unique_ptr<Block> b);
    };

    class ReturnStmt : public Stmt {
    public:
        std::unique_ptr<Expr> value;
        explicit ReturnStmt(std::unique_ptr<Expr> v = nullptr);
    };

    class BreakStmt : public Stmt {};

    class ContinueStmt : public Stmt {};

    struct Parameter {
        std::string name;
    };

    class FunctionDecl : public Stmt {
    public:
        std::string name;
        std::vector<Parameter> parameters;
        std::unique_ptr<Block> body;
        bool is_async;

        FunctionDecl(std::string n, std::vector<Parameter> params,
                     std::unique_ptr<Block> b, bool async = false);
    };



    class Module : public Stmt {
    public:
        std::vector<std::unique_ptr<Stmt>> statements;
        explicit Module(std::vector<std::unique_ptr<Stmt>> stmts);
    };

    // Type system
    enum class TypeKind { INT, FLOAT, BOOL, LIST, DICT, STRUCT, CLASS};

    struct Type {
        TypeKind kind;
        std::string name;  // For struct/class
    };

    // Field definition
    struct FieldDecl {
        std::string                name;
        std::optional<std::string> type_annotation;
        std::unique_ptr<Expr>      default_value;

        // Explicitly declare move — required because unique_ptr deletes copy
        FieldDecl() = default;
        FieldDecl(FieldDecl&&) noexcept = default;
        FieldDecl& operator=(FieldDecl&&) noexcept = default;

        // Explicitly delete copy — makes intent clear
        FieldDecl(const FieldDecl&) = delete;
        FieldDecl& operator=(const FieldDecl&) = delete;
    };

    class StructDecl : public Stmt {
    public:
        std::string name;
        std::vector<FieldDecl> fields;

        // Constructor
        StructDecl(std::string n, std::vector<FieldDecl> f);
    };

    // Class declaration
    class ClassDecl : public Stmt {
    public:
        std::string                                name;
        std::vector<FieldDecl>                     fields;
        std::vector<std::unique_ptr<FunctionDecl>> methods;

        // Constructor
        ClassDecl(std::string n,
                  std::vector<FieldDecl> f,
                  std::vector<std::unique_ptr<FunctionDecl>> m);

        // Move only — both members contain unique_ptr
        ClassDecl() = default;
        ClassDecl(ClassDecl&&) noexcept = default;
        ClassDecl& operator=(ClassDecl&&) noexcept = default;

        // Delete copy
        ClassDecl(const ClassDecl&) = delete;
        ClassDecl& operator=(const ClassDecl&) = delete;
    };

    struct NamedArg {
        std::string name;
        std::unique_ptr<Expr> value;

        NamedArg() = default;
        NamedArg(NamedArg&&) noexcept = default;
        NamedArg& operator=(NamedArg&&) noexcept = default;
        NamedArg(const NamedArg&) = delete;
        NamedArg& operator=(const NamedArg&) = delete;
    };

    // Memberwise initializer call
    // struct InitializerExpr : public Expr {
    //     std::string type_name;  // "Point", "VideoMode", etc.
    //     std::vector<NamedArg> arguments;  // e.g., {{"x", 10.0}, {"y", 20.0}}
    // };

    // ─── NOW InitializerExpr can use it ───────────────────────────────
    class InitializerExpr : public Expr {
    public:
        std::string type_name;
        std::vector<NamedArg> arguments;  // ✅ Now complete type

        InitializerExpr(std::string type, std::vector<NamedArg> args);
    };



    // // Optional type
    // struct OptionType {
    //     llvm::Type inner_type;  // Type wrapped in Option
    // };

    // Some/None
    class SomeExpr : public Expr {
    public:
        std::unique_ptr<Expr> value;
        explicit SomeExpr(std::unique_ptr<Expr> v);
    };

    class NoneExpr : public Expr {
    public:
        NoneExpr() = default;  // ← inline default is fine
    };


    // Field assignment: obj.field = value
    class FieldAssignment : public Stmt {
    public:
        std::unique_ptr<Expr> object;
        std::string field_name;
        std::unique_ptr<Expr> value;

        FieldAssignment(std::unique_ptr<Expr> obj, std::string field, std::unique_ptr<Expr> val);
    };

    // Index assignment: arr[index] = value
    class IndexAssignment : public Stmt {
    public:
        std::unique_ptr<Expr> object;
        std::unique_ptr<Expr> index;
        std::unique_ptr<Expr> value;

        IndexAssignment(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx, std::unique_ptr<Expr> val);
    };

}
