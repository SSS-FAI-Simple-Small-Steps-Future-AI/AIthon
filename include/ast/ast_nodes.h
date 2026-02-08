#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace aithon::ast {

enum class NodeType {
    MODULE,
    FUNCTION_DEF,
    ASYNC_FUNCTION_DEF,
    CLASS_DEF,
    RETURN,
    ASSIGN,
    EXPR,
    BINOP,
    UNARYOP,
    COMPARE,
    CALL,
    AWAIT,
    ATTRIBUTE,
    SUBSCRIPT,
    NAME,
    CONSTANT,
    IF,
    WHILE,
    FOR,
    ASYNC_FOR,
    WITH,
    ASYNC_WITH,
    PASS,
    BREAK,
    CONTINUE,
};

enum class BinaryOp {
    ADD, SUB, MULT, DIV, MOD, POW,
    LSHIFT, RSHIFT, BITOR, BITXOR, BITAND, FLOORDIV
};

enum class UnaryOp {
    INVERT, NOT, UADD, USUB
};

enum class CompareOp {
    EQ, NOTEQ, LT, LTE, GT, GTE, IS, ISNOT, IN, NOTIN
};

class ASTNode {
public:
    NodeType type;
    int lineno;
    int col_offset;
    
    virtual ~ASTNode() = default;
    
protected:
    ASTNode(NodeType t) : type(t), lineno(0), col_offset(0) {}
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

class Module : public ASTNode {
public:
    std::vector<ASTNodePtr> body;
    
    Module() : ASTNode(NodeType::MODULE) {}
};

class FunctionDef : public ASTNode {
public:
    std::string name;
    std::vector<std::string> args;
    std::vector<ASTNodePtr> body;
    ASTNodePtr returns;  // Return type annotation
    bool is_async;
    
    FunctionDef(std::string n, bool async = false)
        : ASTNode(async ? NodeType::ASYNC_FUNCTION_DEF : NodeType::FUNCTION_DEF),
          name(std::move(n)), is_async(async) {}
};

class Return : public ASTNode {
public:
    ASTNodePtr value;
    
    Return() : ASTNode(NodeType::RETURN) {}
};

class Assign : public ASTNode {
public:
    std::vector<ASTNodePtr> targets;
    ASTNodePtr value;
    
    Assign() : ASTNode(NodeType::ASSIGN) {}
};

class Expr : public ASTNode {
public:
    ASTNodePtr value;
    
    Expr() : ASTNode(NodeType::EXPR) {}
};

class BinOp : public ASTNode {
public:
    ASTNodePtr left;
    BinaryOp op;
    ASTNodePtr right;
    
    BinOp() : ASTNode(NodeType::BINOP), op(BinaryOp::ADD) {}
};

class UnaryOpNode : public ASTNode {
public:
    UnaryOp op;
    ASTNodePtr operand;

    UnaryOpNode() : ASTNode(NodeType::UNARYOP), op(UnaryOp::NOT) {}
};

class Compare : public ASTNode {
public:
    ASTNodePtr left;
    std::vector<CompareOp> ops;
    std::vector<ASTNodePtr> comparators;
    
    Compare() : ASTNode(NodeType::COMPARE) {}
};

class Call : public ASTNode {
public:
    ASTNodePtr func;
    std::vector<ASTNodePtr> args;
    
    Call() : ASTNode(NodeType::CALL) {}
};

class Await : public ASTNode {
public:
    ASTNodePtr value;
    
    Await() : ASTNode(NodeType::AWAIT) {}
};

class Attribute : public ASTNode {
public:
    ASTNodePtr value;
    std::string attr;
    
    Attribute() : ASTNode(NodeType::ATTRIBUTE) {}
};

class Subscript : public ASTNode {
public:
    ASTNodePtr value;
    ASTNodePtr slice;
    
    Subscript() : ASTNode(NodeType::SUBSCRIPT) {}
};

class Name : public ASTNode {
public:
    std::string id;
    
    Name() : ASTNode(NodeType::NAME) {}
    explicit Name(std::string name) : ASTNode(NodeType::NAME), id(std::move(name)) {}
};

class Constant : public ASTNode {
public:
    using Value = std::variant<int64_t, double, std::string, bool>;
    Value value;
    
    Constant() : ASTNode(NodeType::CONSTANT), value(0L) {}
    explicit Constant(Value val) : ASTNode(NodeType::CONSTANT), value(std::move(val)) {}
};

class If : public ASTNode {
public:
    ASTNodePtr test;
    std::vector<ASTNodePtr> body;
    std::vector<ASTNodePtr> orelse;
    
    If() : ASTNode(NodeType::IF) {}
};

class While : public ASTNode {
public:
    ASTNodePtr test;
    std::vector<ASTNodePtr> body;
    std::vector<ASTNodePtr> orelse;
    
    While() : ASTNode(NodeType::WHILE) {}
};

class For : public ASTNode {
public:
    ASTNodePtr target;
    ASTNodePtr iter;
    std::vector<ASTNodePtr> body;
    std::vector<ASTNodePtr> orelse;
    bool is_async;
    
    For() : ASTNode(NodeType::FOR), is_async(false) {}
};

class Pass : public ASTNode {
public:
    Pass() : ASTNode(NodeType::PASS) {}
};

class Break : public ASTNode {
public:
    Break() : ASTNode(NodeType::BREAK) {}
};

class Continue : public ASTNode {
public:
    Continue() : ASTNode(NodeType::CONTINUE) {}
};

} // namespace pyvm::ast