#pragma once
#include <vector>
#include <string>
#include <memory>
#include "lexer.hpp"
#include "ast.hpp"

class Parser {
public:
    explicit Parser(const std::vector<Token>& toks);

    Program parseProgram();

private:
    const std::vector<Token>& tokens;
    size_t pos = 0;

    // Helpers
    const Token& peek(int off = 0) const;
    bool eof() const;
    bool match(TokenType t, const std::string& v = "");
    const Token& expect(TokenType t, const std::string& v = "");

    // Declarations
    DeclPtr parseDecl();
    std::unique_ptr<FuncDecl> parseFunc();

    // Statements
    StmtPtr parseStmt();
    std::unique_ptr<ReturnStmt> parseReturn();
    StmtPtr parseAssignStmt();
    std::vector<StmtPtr> parseBlock();

    // Expressions
    ExprPtr parseIndentiferFamily(ExprPtr identifier);
    ExprPtr parseRHS();
    ExprPtr parsePrimitive();
    ExprPtr parseCallExpr(ExprPtr identifier);

    // Utils
    bool isBinaryOp(const std::string& op) const;
};
