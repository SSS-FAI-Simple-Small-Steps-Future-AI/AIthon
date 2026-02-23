#include "../../include/parser/ast.h"
#include <llvm/IR/Type.h>

namespace aithon::parser::ast {
    struct NamedArg;

// ============================================================================
// Expression Implementations
// ============================================================================

IntegerLiteral::IntegerLiteral(long long v) : value(v) {}

FloatLiteral::FloatLiteral(double v) : value(v) {}

StringLiteral::StringLiteral(std::string v) : value(std::move(v)) {}

BoolLiteral::BoolLiteral(bool v) : value(v) {}

Identifier::Identifier(std::string n) : name(std::move(n)) {}

BinaryOp::BinaryOp(Op o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
    : op(o), left(std::move(l)), right(std::move(r)) {}

UnaryOp::UnaryOp(Op o, std::unique_ptr<Expr> operand)
    : op(o), operand(std::move(operand)) {}

CallExpr::CallExpr(std::unique_ptr<Expr> c, std::vector<std::unique_ptr<Expr>> args)
    : callee(std::move(c)), arguments(std::move(args)) {}

IndexExpr::IndexExpr(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx)
    : object(std::move(obj)), index(std::move(idx)) {}

MemberExpr::MemberExpr(std::unique_ptr<Expr> obj, std::string mem)
    : object(std::move(obj)), member(std::move(mem)) {}

ListExpr::ListExpr(std::vector<std::unique_ptr<Expr>> elems)
    : elements(std::move(elems)) {}

DictExpr::DictExpr(std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> p)
    : pairs(std::move(p)) {}

// ============================================================================
// Statements, Conditions, Loops - Implementations
// ============================================================================

ExprStmt::ExprStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {}

Assignment::Assignment(std::string n, std::unique_ptr<Expr> v)
    : name(std::move(n)), value(std::move(v)) {}

Block::Block(std::vector<std::unique_ptr<Stmt>> stmts)
    : statements(std::move(stmts)) {}

IfStmt::IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Block> then_b, 
               std::unique_ptr<Block> else_b)
    : condition(std::move(cond)), then_block(std::move(then_b)), 
      else_block(std::move(else_b)) {}

WhileStmt::WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Block> b)
    : condition(std::move(cond)), body(std::move(b)) {}

ForStmt::ForStmt(std::string var, std::unique_ptr<Expr> iter, std::unique_ptr<Block> b)
    : variable(std::move(var)), iterable(std::move(iter)), body(std::move(b)) {}

ReturnStmt::ReturnStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}

FunctionDecl::FunctionDecl(std::string n, std::vector<Parameter> params, 
                           std::unique_ptr<Block> b, bool async)
    : name(std::move(n)), parameters(std::move(params)), 
      body(std::move(b)), is_async(async) {}


// ============================================================================
// Structure, Class, Module - Implementations
// ============================================================================

StructDecl::StructDecl(std::string n, std::vector<FieldDecl> f)
: name(std::move(n)), fields(std::move(f)) {}

ClassDecl::ClassDecl(std::string n,
                 std::vector<FieldDecl> f,
                 std::vector<std::unique_ptr<FunctionDecl>> m)
: name(std::move(n)), fields(std::move(f)), methods(std::move(m)) {}


Module::Module(std::vector<std::unique_ptr<Stmt>> stmts)
: statements(std::move(stmts)) {}


InitializerExpr::InitializerExpr(std::string type, std::vector<NamedArg> args)
    : type_name(std::move(type)), arguments(std::move(args)) {}

SomeExpr::SomeExpr(std::unique_ptr<Expr> v) : value(std::move(v)) {}


FieldAssignment::FieldAssignment(std::unique_ptr<Expr> obj, std::string field, std::unique_ptr<Expr> val)
: object(std::move(obj)), field_name(std::move(field)), value(std::move(val)) {}

IndexAssignment::IndexAssignment(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx, std::unique_ptr<Expr> val)
    : object(std::move(obj)), index(std::move(idx)), value(std::move(val)) {}

}
