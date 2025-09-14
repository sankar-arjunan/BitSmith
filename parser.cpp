#include "parser.hpp"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token>& toks) : tokens(toks) {}

const Token& Parser::peek(int off) const {
    static Token dummy{TokenType::END_OF_FILE, "", 0, 0};
    size_t i = pos + off;
    return i < tokens.size() ? tokens[i] : dummy;
}

bool Parser::eof() const { return peek().type == TokenType::END_OF_FILE; }

bool Parser::match(TokenType t, const std::string& v) {
    if (peek().type == t && (v.empty() || peek().value == v)) {
        ++pos;
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType t, const std::string& v) {
    if (peek().type == t && (v.empty() || peek().value == v))
        return tokens[pos++];
    throw std::runtime_error(
        "Unexpected token : " + peek().value + " -> at line and col : " +
        std::to_string(peek().line) + " " + std::to_string(peek().col));
}

// TOP LEVEL ========================================================

Program Parser::parseProgram() {
    Program prog;
    while (!eof()) {
        if (match(TokenType::SEMICOLON)) continue;
        prog.decls.push_back(parseDecl());
    }
    return prog;
}

// DECLARATIONS ========================================================

DeclPtr Parser::parseDecl() {
    if (match(TokenType::FUNCTION)) return parseFunc();
    throw std::runtime_error("Unexpected top level declaration : " + peek().value +
                             " -> at line and col : " + std::to_string(peek().line) +
                             " : " + std::to_string(peek().col));
}

std::unique_ptr<FuncDecl> Parser::parseFunc() {
    auto decl = std::make_unique<FuncDecl>();
    decl->name = expect(TokenType::IDENTIFIER).value;
    if(decl -> name == "main"){
        expect(TokenType::COLON);
        std::string argc;
        int counter = 0;
        for(auto c:expect(TokenType::DATA).value){
            if(counter < 4){counter++; continue;}
            if(c != ')') argc.push_back(c);
        }
        decl->argc = argc;
    }
    else{
        decl -> argc = "-1";
    }
    decl->body = parseBlock();
    return decl;
}

// STATEMENTS ========================================================

StmtPtr Parser::parseStmt() {
    while (match(TokenType::SEMICOLON)) {}
    if (match(TokenType::RETURN)) return parseReturn();
    if (peek().type == TokenType::IDENTIFIER) return parseAssignStmt();
    throw std::runtime_error("Unexpected token in statement: " + peek().value +
                             " -> at line and col : " + std::to_string(peek().line) +
                             " : " + std::to_string(peek().col));
}

std::unique_ptr<ReturnStmt> Parser::parseReturn() {
    auto result = std::make_unique<ReturnStmt>(parsePrimitive());
    expect(TokenType::SEMICOLON);
    return result;
}

std::vector<StmtPtr> Parser::parseBlock() {
    expect(TokenType::OPEN_BRACE);
    std::vector<StmtPtr> stmts;
    while (!eof() && peek().type != TokenType::CLOSE_BRACE) {
        StmtPtr stmt = parseStmt();
        if (stmt) stmts.push_back(std::move(stmt));
        else ++pos;
    }
    expect(TokenType::CLOSE_BRACE);
    return stmts;
}

// EXPRESSIONS ========================================================

StmtPtr Parser::parseAssignStmt(){
    auto identifier = std::make_unique<VarExpr>(expect(TokenType::IDENTIFIER).value);
    auto lhs = parseIndentiferFamily(std::move(identifier));
    expect(TokenType::EQ);
    auto rhs = parseRHS();
    return std::make_unique<AssignStmt>(
        std::move(lhs),
        std::move(rhs)
    );
}

ExprPtr Parser::parseRHS(){
    if (peek().type == TokenType::TILDE){
        std::string op = tokens[pos++].value;
        auto sub = parsePrimitive();
        return std::make_unique<NotExpr>(std::move(sub));
    }
    else if(peek().type == TokenType::IDENTIFIER){
        auto identifier = std::make_unique<VarExpr>(expect(TokenType::IDENTIFIER).value);
        if (peek().type == TokenType::OPEN_PAREN) 
            return parseCallExpr(std::move(identifier));
        auto lop = parseIndentiferFamily(std::move(identifier));
        
        if(peek().type == TokenType::SEMICOLON) return lop;

        if (peek().type == TokenType::CONCAT) {
            std::vector<ExprPtr> operands;
            operands.push_back(std::move(lop));
            while (match(TokenType::CONCAT)) {
                operands.push_back(parsePrimitive());
            }
            if (operands.size() > 1)
                return std::make_unique<ConcatExpr>(std::move(operands));
        }

        std::string op = tokens[pos++].value;
        if(!isBinaryOp(op)) throw std::runtime_error("Invalid binary operator : "+op);
        auto rop = parsePrimitive();

        return std::make_unique<BinaryExpr>(op, std::move(lop), std::move(rop));
    }
    else if(peek().type == TokenType::DATA){
        auto lop = std::make_unique<DataExpr>(expect(TokenType::DATA).value);
        if(peek().type == TokenType::SEMICOLON) return lop;
        if (peek().type == TokenType::CONCAT) {
            std::vector<ExprPtr> operands;
            operands.push_back(std::move(lop));
            while (match(TokenType::CONCAT)) {
                operands.push_back(parsePrimitive());
            }
            if (operands.size() > 1)
                return std::make_unique<ConcatExpr>(std::move(operands));
        }

        std::string op = tokens[pos++].value;
        if(!isBinaryOp(op)) throw std::runtime_error("Invalid bianry operator : "+op);
        auto rop = parsePrimitive();

        return std::make_unique<BinaryExpr>(op, std::move(lop), std::move(rop));
    }
    else throw std::runtime_error("Invalid expression : " + peek().value);
}

ExprPtr Parser::parsePrimitive(){
    if(peek().type == TokenType::IDENTIFIER){
        auto identifier = std::make_unique<VarExpr>(expect(TokenType::IDENTIFIER).value);
        if(peek().type == TokenType::OPEN_PAREN) throw std::runtime_error("Primitive expression cannot have function call");
        return parseIndentiferFamily(std::move(identifier));
    }
    else if(peek().type == TokenType::DATA){
        return std::make_unique<DataExpr>(expect(TokenType::DATA).value);
    }
    else throw std::runtime_error("Primitive expression invalid "+peek().value);
}

ExprPtr Parser::parseIndentiferFamily(ExprPtr identifier){
    if(peek().type == TokenType::SEMICOLON) return identifier;
    ExprPtr node = std::move(identifier);
    if(match(TokenType::OPEN_SQR)){
        std::string start;
        std::string end;
        if (peek().type == TokenType::COLON || peek().type == TokenType::CLOSE_SQR) {
            start = "bit(0)";
        } else {
            start =  expect(TokenType::DATA).value;
        }
        if (match(TokenType::COLON)) {
            if (peek().type == TokenType::CLOSE_SQR) {
                end = "bit(-1)";
            } else {
                end = expect(TokenType::DATA).value;
            }
            expect(TokenType::CLOSE_SQR);
            node = std::make_unique<SliceExpr>(std::move(node), start, end);
        } else {
            expect(TokenType::CLOSE_SQR);
            node = std::make_unique<IndexExpr>(std::move(node), start);
        }
    }
    return node;
}


ExprPtr Parser::parseCallExpr(ExprPtr identifier){
    expect(TokenType::OPEN_PAREN);
    ExprPtr arg = parsePrimitive();
    expect(TokenType::CLOSE_PAREN);
    return std::make_unique<CallExpr>(std::move(identifier), std::move(arg));
}

bool Parser::isBinaryOp(const std::string& op) const {
    static const std::vector<std::string> ops = {
        "|", "&", "^",
        "==", "!=", "&&", "||", "<<", ">>",
        "<<<", ">>>"
    };

    for (auto& o : ops) {
        if (o == op) return true;
    }
    return false;
}
