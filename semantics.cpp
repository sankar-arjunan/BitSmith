#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include "semantics.hpp"

std::unordered_map<std::string, std::vector<int>> varMapping;
std::unordered_map<std::string, FuncDecl*> funcMapping;
std::unordered_map<int, Bit> bitMapping;
int nextBitIndex = 0;

void printDebug() {

    std::cout << "=== varMapping ===\n";
    for (auto &p : varMapping) {
        std::cout << p.first << " : ";
        for(auto x : p.second){
            std::cout << std::to_string(x) << " ";
        }
    }

    std::cout << "=== bitMapping ===\n";
    for (auto &p : bitMapping) {
        int i = p.first;
        Bit &b = p.second;
        std::cout << i << " : ";
        if (b.op.empty() && b.lhs == -1 && b.rhs == -1) {
            std::cout << (b.value ? "1" : "0");
        } else if (!b.op.empty() && b.op == "~") {
            std::cout << "~ " << b.lhs;
        } else if (!b.op.empty()) {
            std::cout << b.lhs << " " << b.op << " " << b.rhs;
        } else {
            std::cout << "unknown";
        }
        std::cout << "\n";
    }
}

std::vector<int> SemanticAnalyzer::processPrimitive(Expr* expr) {
    std::vector<int> indices;

    if (auto ve = dynamic_cast<VarExpr*>(expr)) {
        auto it = varMapping.find(ve->name);
        if (it == varMapping.end()) throw std::runtime_error("Unknown variable: " + ve->name);
        indices =  it->second;
    }

    else if (auto se = dynamic_cast<SliceExpr*>(expr)) {
        if (auto cvar = dynamic_cast<VarExpr*>(se->container.get())) {
            auto &parent = varMapping[cvar->name];
            int start = std::stoi(se->start.substr(4, se->start.size() - 5));
            int end = (se->end == "-1") ? static_cast<int>(parent.size()) : std::stoi(se->end.substr(4, se->end.size() - 5));
            if (start < 0) start = static_cast<int>(parent.size()) + start;
            if (end < 0) end = static_cast<int>(parent.size()) + end;
            
            if (start < 0 || end < start || end > static_cast<int>(parent.size())) 
                throw std::runtime_error("Invalid slice indices");
            for (int i = start; i < end; ++i) indices.push_back(parent[i]);
        }
        else throw std::runtime_error("Slice container is not a variable");
    }

    else if (auto de = dynamic_cast<DataExpr*>(expr)) {
        std::string s = de->value;

        if (s.substr(0, 4) == "hex(" && s.back() == ')') {
            s = s.substr(4, s.size() - 5);
            unsigned long long val = std::stoull(s, nullptr, 16);
            
            int bitsNeeded = s.size() * 4;  

            for (int i = bitsNeeded - 1; i >= 0; --i) {
                int value = (val & (1ULL << i)) != 0 ? 1 : 0;
                indices.push_back(value);
            }
        }

        if (s.substr(0, 4) == "bit(" && s.back() == ')') {
            std::string bits = s.substr(4, s.size() - 5);
            for (char c : bits) {

                int value = (c=='0') ? 0 : 1;
                indices.push_back(value);
            }
        }
    }

    else if (auto ie = dynamic_cast<IndexExpr*>(expr)) {
        if (auto cvar = dynamic_cast<VarExpr*>(ie->container.get())) {
            auto &parent = varMapping[cvar->name];
            int idx = std::stoi(ie->index.substr(4, ie->index.size() - 5));
            if (idx < 0) idx = static_cast<int>(parent.size()) + idx;
            if (idx < 0 || idx >= static_cast<int>(parent.size())) throw std::runtime_error("Invalid index");
            indices.push_back(parent[idx]);
        }
        else throw std::runtime_error("Index container is not a variable");
    }

    else if (auto ce = dynamic_cast<ConcatExpr*>(expr)) {
        for (auto &op : ce->operands) {
            auto sub = processPrimitive(op.get());
            indices.insert(indices.end(), sub.begin(), sub.end());
        }
    }

    else if (auto be = dynamic_cast<BinaryExpr*>(expr)) {
        auto L = processPrimitive(be->lhs.get());

        if (be->op == "^" || be->op == "&" || be->op == "|") {
            auto R = processPrimitive(be->rhs.get());
            size_t n = std::max(L.size(), R.size());
            for (size_t i = 0; i < n; ++i) {
                int li = (i < L.size()) ? L[i] : 0;
                int ri = (i < R.size()) ? R[i] : 0;


                Bit nb;
                nb.value = false;
                nb.lhs = li;
                nb.rhs = ri;
                nb.op = be->op;
                std::string op = be -> op;


                if(be->op == "&"){
                    if(li == 0 || ri == 0){
                        indices.push_back(0);
                    }
                    else if(li == 1 && ri == 1){
                        indices.push_back(1);
                    }
                    else if(li == 1){
                        indices.push_back(ri);
                    }
                    else if(ri == 1){ 
                        indices.push_back(li);
                    }
                    else{      
                        int ni = nextBitIndex++;
                        bitMapping[ni] = nb;
                        indices.push_back(ni);
                    }
                }
                if(be->op == "|"){
                    
                    if(li == 1 || ri == 1){
                        indices.push_back(1);
                    }
                    else if(li == 0 && ri == 0){
                        indices.push_back(0);
                    }
                    else if(li == 0){
                        indices.push_back(ri);
                    }
                    else if(ri == 0){
                        
                        indices.push_back(li);
                    }
                    else{ 
                        int ni = nextBitIndex++;
                        bitMapping[ni] = nb;
                        indices.push_back(ni);
                    }
                }
                if(be->op == "^"){

                    if(li == ri){
                        indices.push_back(0);
                    }
                    else if(((li == 0 && ri ==1 ) || (li == 1 && ri == 0) ) && li != ri){
                        indices.push_back(1);
                    }
                    else{
                        int ni = nextBitIndex++;
                        bitMapping[ni] = nb;
                        indices.push_back(ni);
                    }
                }

            }

        }

        else if (auto rhsVar = dynamic_cast<DataExpr*>(be->rhs.get())) {
            if (rhsVar->value.rfind("bit(", 0) == 0 && rhsVar->value.back() == ')') {
                int num = std::stoi(rhsVar->value.substr(4, rhsVar->value.size() - 5));

                if (be->op == ">>") {
                    if (num > 0 && num < (int)L.size()) {
                        std::vector<int> shifted(L.size(), 0);
                        for (int i = 0; i < (int)L.size() - num; i++) {
                            shifted[i + num] = L[i];
                        }
                        L.swap(shifted);
                    } else {
                        std::fill(L.begin(), L.end(), 0);
                    }
                }
                else if (be->op == "<<") {
                    if (num > 0 && num < (int)L.size()) {
                        std::vector<int> shifted(L.size(), 0);
                        for (int i = num; i < (int)L.size(); i++) {
                            shifted[i - num] = L[i];
                        }
                        L.swap(shifted);
                    } else {
                        std::fill(L.begin(), L.end(), 0);
                    }
                }
                else if (be->op == ">>>") {
                    if (num > 0 && !L.empty()) {
                        num %= L.size();
                        std::rotate(L.rbegin(), L.rbegin() + num, L.rend());
                    }
                }
                else if (be->op == "<<<") {
                    if (num > 0 && !L.empty()) {
                        num %= L.size();
                        std::rotate(L.begin(), L.begin() + num, L.end());
                    }
                }

            }

            indices = L;
        }
    }


    else if (auto ne = dynamic_cast<NotExpr*>(expr)) {
        auto S = processPrimitive(ne->expr.get());
        for (int sidx : S) {
            Bit nb;
            nb.value = false;
            nb.lhs = sidx;
            nb.rhs = -1;
            nb.op = "~";
            if(nb.lhs == 0 || nb.lhs == 1){
                indices.push_back((nb.lhs+1) % 2);
            }
            else {
                int ni = nextBitIndex++;
                bitMapping[ni] = nb;
                indices.push_back(ni);
            }
        }
    }

    else if (auto ce = dynamic_cast<CallExpr*>(expr)) {
        if (auto calleeVar = dynamic_cast<VarExpr*>(ce->callee.get())) {
            auto fit = funcMapping.find(calleeVar->name);
            if (fit == funcMapping.end()) throw std::runtime_error("Unknown function: " + calleeVar->name);
            std::vector<int> arg = processPrimitive(ce->arg.get());
            return processFunction(*(fit->second), arg);
        }
        else throw std::runtime_error("Call target is not a simple var");
    }
    else throw std::runtime_error("Invalid primitive");

    for(int ind = 0; ind < indices.size(); ind++){
        int v = indices[ind];
        Bit b = bitMapping[v];
        if((b.lhs == 0 || b.lhs == 1) && (b.rhs == -1) && (b.op == "")){
            indices[ind] = b.lhs;
        }
    }
    return indices;
}

std::vector<int> SemanticAnalyzer::processFunction(FuncDecl& function, std::vector<int>& inputIndices) {
    varMapping[function.name] = inputIndices;
    for (auto &stmt : function.body) {
        if (auto asgn = dynamic_cast<AssignStmt*>(stmt.get())) {
            Expr* lhs = asgn->lhs.get();
            Expr* rhs = asgn->rhs.get();
            std::vector<int> rhsIndices = processPrimitive(rhs);

            if (auto lhsVar = dynamic_cast<VarExpr*>(lhs)) {
                varMapping[lhsVar->name] = rhsIndices;
                continue;
            }

            if (auto lhsSlice = dynamic_cast<SliceExpr*>(lhs)) {
                if (auto cvar = dynamic_cast<VarExpr*>(lhsSlice->container.get())) {
                    auto &parent = varMapping[cvar->name];
                    int start = std::stoi(lhsSlice->start.substr(4, lhsSlice->start.size() - 5));
                    int end = (lhsSlice->end == "-1") ? static_cast<int>(parent.size()) : std::stoi(lhsSlice->end.substr(4, lhsSlice->end.size() - 5));
                    if (start < 0) start = static_cast<int>(parent.size()) + start;
                    if (end < 0) end = static_cast<int>(parent.size()) + end;
                    if (start < 0 || end < start || end > static_cast<int>(parent.size())) throw std::runtime_error("Invalid slice indices");
                    for (int i = start; i < end; ++i) {
                        int src = (i - start < static_cast<int>(rhsIndices.size())) ? rhsIndices[i - start] : -1;
                        if (src != -1) parent[i] = src;
                        else {
                            Bit nb; nb.op = ""; nb.lhs = nb.rhs = -1; nb.value = false;
                            int ni = nextBitIndex++;
                            bitMapping[ni] = nb;
                            parent[i] = ni;
                        }
                    }
                    varMapping[cvar->name] = parent;
                    continue;
                }
                else throw std::runtime_error("Slice target container not a variable");
            }

            if (auto lhsIndex = dynamic_cast<IndexExpr*>(lhs)) {
                if (auto cvar = dynamic_cast<VarExpr*>(lhsIndex->container.get())) {
                    auto &parent = varMapping[cvar->name];
                    int idx = std::stoi(lhsIndex->index.substr(4, lhsIndex->index.size() - 5));
                    if (idx < 0) idx = static_cast<int>(parent.size()) + idx;
                    if (idx < 0 || idx >= static_cast<int>(parent.size())) throw std::runtime_error("Invalid index on LHS");
                    int src = (rhsIndices.empty() ? -1 : rhsIndices[0]);
                    if (src != -1) parent[idx] = src;
                    else {
                        Bit nb; nb.op = ""; nb.lhs = nb.rhs = -1; nb.value = false;
                        int ni = nextBitIndex++;
                        bitMapping[ni] = nb;
                        parent[idx] = ni;
                    }
                    varMapping[cvar->name] = parent;
                    continue;
                }
                else throw std::runtime_error("Index target container not a variable");
            }

            else throw std::runtime_error("Unsupported LHS in assignment");
        }

        if (auto ret = dynamic_cast<ReturnStmt*>(stmt.get())) {
            return processPrimitive(ret->value.get());
        }
    }
    return {};
}

std::vector<int> SemanticAnalyzer::analyze(Program* root) {
    funcMapping.clear();
    for (auto &decl : root->decls) {
        if (auto f = dynamic_cast<FuncDecl*>(decl.get())) 
            funcMapping[f->name] = f;

        else throw std::runtime_error("Invalid top-level declaration");
    }

    auto it = funcMapping.find("main");
    if (it == funcMapping.end()) throw std::runtime_error("No 'main' function defined");
    FuncDecl &mainFunc = *(it->second);

    std::string argcStr = mainFunc.argc;
    if (argcStr == "-1") throw std::runtime_error("No valid argc for main()");
    int argc = 0;
    try { argc = std::stoi(argcStr); }
    catch (...) { throw std::runtime_error("Invalid argc: not a number"); }
    if (argc <= 0) throw std::runtime_error("Invalid argc: must be > 0");

    bitMapping.clear();
    for (int i = 0; i < argc + 2; ++i) {
        Bit b;
        b.lhs = b.rhs = -1;
        b.op = "";
        if (i == 0) b.value = false;
        else if (i == 1) b.value = true;
        else b.value = false;
        bitMapping[nextBitIndex++] = b;
    }

    std::vector<int> inputIndices;
    for (int i = 2; i < argc + 2; ++i) inputIndices.push_back(i);


    std::vector<int> result = processFunction(mainFunc, inputIndices);
    // printDebug();
    // for(auto id : result){
    //     std::cout << id << "\n";
    // }
    
    return result;
}


std::string SemanticAnalyzer::cGen(const std::string& name, std::vector<int> out) {
    auto it = funcMapping.find("main");
    if (it == funcMapping.end()) throw std::runtime_error("No 'main' function defined");
    FuncDecl &mainFunc = *(it->second);

    std::string argcStr = mainFunc.argc;
    if (argcStr == "-1") throw std::runtime_error("No valid argc for main()");
    int argc = 0;
    try { argc = std::stoi(argcStr); }
    catch (...) { throw std::runtime_error("Invalid argc: not a number"); }
    if (argc <= 0) throw std::runtime_error("Invalid argc: must be > 0");

    int inpBits =argc;
    int inpBytes = (inpBits + 7) / 8;
    std::string code = "char* " + name + "(char* input) {\n";
    int outBytes = (out.size() + 7) / 8;
    code += "    static char output[" + std::to_string(outBytes) + "] = {0};\n";
    code += "    for (int i = 0; i < "+ std::to_string(outBytes) +"; i++) output[i] = 0;\n";

    for (size_t i = 0; i < out.size(); ++i) {
        int idx = out[i] - 2;
        std::string bitExpr;

        if (idx < 0) {
            bitExpr = (idx == -2) ? "(0)" : "(1)";
            if(idx == -2) continue;
        }
        else if (idx < inpBits) {
            bitExpr = "((input[" + std::to_string(idx / 8) + "] >> " 
                     + std::to_string(7 - (idx % 8)) + ") & 1)";
        }
        else {
            auto b = bitMapping[idx + 2];
            auto lhsInd = b.lhs - 2;
            auto bitExpr1 = "((input[" + std::to_string(lhsInd / 8) + "] >> " 
                           + std::to_string(7 - (lhsInd % 8)) + ") & 1)";
            if (b.op == "~") {
                bitExpr = "(~" + bitExpr1 + " & 1)";
            } else {
                auto rhsInd = b.rhs - 2;
                auto bitExpr2 = "((input[" + std::to_string(rhsInd / 8) + "] >> " 
                               + std::to_string(7 - (rhsInd % 8)) + ") & 1)";
                bitExpr = "((" + bitExpr1 + " " + b.op + " " + bitExpr2 + ") & 1)";
            }
        }


        code += "    output[" + std::to_string(i / 8) + "] |= (" + bitExpr + " << " + std::to_string((7-(i%8))) + ");\n";
    }

    code += "\n    return output;\n";
    code += "}\n";

    return code;
}
