#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "ast.hpp"

struct Bit {
    bool value;
    int lhs;
    int rhs;
    std::string op;
};

class SemanticAnalyzer {
public:
    std::vector<int> analyze(Program* root);
    std::vector<int> processPrimitive(Expr* expr);
    std::vector<int> processFunction(FuncDecl& function, std::vector<int>& inputIndices);
    std::string cGen(const std::string& name, std::vector<int> out);
};

extern std::unordered_map<std::string, std::vector<int>> varMapping;
extern std::unordered_map<std::string, FuncDecl*> funcMapping;
extern std::unordered_map<int, Bit> bitMapping;
extern int nextBitIndex;

void printDebug();
