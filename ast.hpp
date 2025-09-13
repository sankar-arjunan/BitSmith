#pragma once
#include <string>
#include <vector>
#include <memory>

struct Expr;
struct Stmt;
struct Decl;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using DeclPtr = std::unique_ptr<Decl>;


//===============EXPRESSIONS===============//

struct Expr {
    virtual ~Expr() = default;
};

struct DataExpr : Expr {
    std::string value;
    DataExpr(const std::string& v) : value(v) {}
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(const std::string& n) : name(n) {}
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr lhs, rhs;
    BinaryExpr(const std::string& o, ExprPtr l, ExprPtr r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct ConcatExpr : Expr {
    std::vector<ExprPtr> operands;
    ConcatExpr(std::vector<ExprPtr> o) : operands(std::move(o)) {}
};

struct NotExpr : Expr {
    ExprPtr expr;
    NotExpr(ExprPtr e) : expr(std::move(e)) {}
};

struct CallExpr : Expr {
    ExprPtr callee;
    ExprPtr arg;
    CallExpr(ExprPtr c, ExprPtr a) : callee(std::move(c)), arg(std::move(a)) {}
};

struct IndexExpr : Expr {
    ExprPtr container;
    std::string index;
    IndexExpr(ExprPtr c, const std::string& i) : container(std::move(c)), index(i) {}
};

struct SliceExpr : Expr {
    ExprPtr container;
    std::string start, end;
    SliceExpr(ExprPtr c, const std::string& s, const std::string& e)
        : container(std::move(c)), start(s), end(e) {}
};

//===============STATEMENTS===============//

struct Stmt {
    virtual ~Stmt() = default;
};

struct AssignStmt : Stmt {
    ExprPtr lhs;
    ExprPtr rhs;
    AssignStmt(ExprPtr lhs, ExprPtr rhs) : lhs(std::move(lhs)) , rhs(std::move(rhs)){}
};

struct ReturnStmt : Stmt {
    ExprPtr value;
    ReturnStmt(ExprPtr v) : value(std::move(v)) {}
};

//===============DECLARATIONS===============//

struct Decl {
    virtual ~Decl() = default;
};

struct FuncDecl : Decl {
    std::string name;
    std::vector<StmtPtr> body;
    std::string argc;

    FuncDecl() = default;
    FuncDecl(const std::string& n,
             std::vector<StmtPtr> b, const std::string& a)
        : name(n), body(std::move(b)), argc(a){}
};

struct Program {
    std::vector<DeclPtr> decls;
};
