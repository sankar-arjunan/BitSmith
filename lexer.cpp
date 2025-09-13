#include <cctype>
#include <stdexcept>
#include "lexer.hpp"

Lexer::Lexer(const std::string &src) : source(src) {}

bool Lexer::eof() const { return pos >= source.size(); }

char Lexer::peek() const { return eof() ? '\0' : source[pos]; }

char Lexer::peekN(size_t n) const { return (pos + n >= source.size()) ? '\0' : source[pos + n]; }

char Lexer::get() {
    if (eof()) return '\0';
    char c = source[pos++];
    if (c == '\n') {
        ++line;
        col = 1;
    } else {
        ++col;
    }
    return c;
}

Token Lexer::makeToken(TokenType t, const std::string &v, int tl, int tc) const {
    return Token{t, v, tl, tc};
}

void Lexer::skipWhitespace() {
    while (!eof()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            get();
            continue;
        }
        break;
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    while (true) {
        skipWhitespace();
        if (eof()) break;
        int tl = line, tc = col;
        char c = get();

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string id(1, c);
            while (!eof() && (std::isalnum(peek()) || peek() == '_')) id.push_back(get());
            auto kwIt = keywords.find(id);
            if (kwIt != keywords.end())
                out.push_back(makeToken(kwIt->second, id, tl, tc));
            else
                out.push_back(makeToken(TokenType::IDENTIFIER, id, tl, tc));
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num(1, c);
            if (c == '0' && (peek() == 'x' || peek() == 'X')) {
                get();
                num = "hex(";
                while (!eof() && std::isxdigit(peek())) {
                    num.push_back(get());
                }
                num += ')';
            } else {
                while (!eof() && std::isdigit(peek())) num.push_back(get());
                num = "bit(" + num + ")";
            }

            out.push_back(makeToken(TokenType::DATA, num, tl, tc));
            continue;
        }

        std::string four;
        for (int i = 0; i < 4 && pos + i - 1 < source.size(); ++i) four.push_back(i == 0 ? c : source[pos + i - 1]);
        if (four.size() >= 3) {
            std::string three = four.substr(0, 3);
            auto it3 = twoCharOps.find(three);
            if (it3 != twoCharOps.end()) {
                get();
                get();
                out.push_back(makeToken(it3->second, three, tl, tc));
                continue;
            }
        }
        if (four.size() >= 4) {
            std::string fourStr = four.substr(0, 4);
            auto it4 = twoCharOps.find(fourStr);
            if (it4 != twoCharOps.end()) {
                get();
                get();
                get();
                out.push_back(makeToken(it4->second, fourStr, tl, tc));
                continue;
            }
        }

        std::string two(1, c);
        two.push_back(peek());
        auto twoIt = twoCharOps.find(two);
        if (twoIt != twoCharOps.end()) {
            get();
            out.push_back(makeToken(twoIt->second, two, tl, tc));
            continue;
        }

        auto oneOp = singleCharOps.find(c);
        if (oneOp != singleCharOps.end()) {
            out.push_back(makeToken(oneOp->second, std::string(1, c), tl, tc));
            continue;
        }

        auto punct = punctuation.find(c);
        if (punct != punctuation.end()) {
            out.push_back(makeToken(punct->second, std::string(1, c), tl, tc));
            continue;
        }

        throw std::runtime_error("Unexpected character '" + std::string(1, c) + "'");
    }
    out.push_back(makeToken(TokenType::END_OF_FILE, "", line, col));
    return out;
}

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"function", TokenType::FUNCTION},
    {"return", TokenType::RETURN}
};

const std::unordered_map<std::string, TokenType> Lexer::twoCharOps = {
    {"==", TokenType::EQ_2},
    {"!=", TokenType::NOT_EQ},
    {"&&", TokenType::AND},
    {"||", TokenType::OR},
    {"<<", TokenType::SL},
    {">>", TokenType::SR},
    {"<<<", TokenType::CSL},
    {">>>", TokenType::CSR},
    {"::", TokenType::CONCAT}
};

const std::unordered_map<char, TokenType> Lexer::singleCharOps = {
    {'=', TokenType::EQ},
    {'|', TokenType::PIPE},
    {'&', TokenType::AMP},
    {'^', TokenType::XOR},
    {'~', TokenType::TILDE}
};

const std::unordered_map<char, TokenType> Lexer::punctuation = {
    {':', TokenType::COLON},
    {';', TokenType::SEMICOLON},
    {'{', TokenType::OPEN_BRACE},
    {'}', TokenType::CLOSE_BRACE},
    {'(', TokenType::OPEN_PAREN},
    {')', TokenType::CLOSE_PAREN},
    {'[', TokenType::OPEN_SQR},
    {']', TokenType::CLOSE_SQR}
};
