#pragma once
#include <string>
#include <vector>
#include <unordered_map>

enum class TokenType {
    FUNCTION, RETURN,
    IDENTIFIER,
    DATA,
    COLON, SEMICOLON,
    OPEN_BRACE, CLOSE_BRACE,
    OPEN_PAREN, CLOSE_PAREN,
    OPEN_SQR, CLOSE_SQR,
    EQ,
    PIPE, AMP, XOR, TILDE,
    EQ_2, NOT_EQ,
    AND, OR,
    SL, SR, CSL, CSR, CONCAT,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line{1}, col{1};
};

class Lexer {
public:
    explicit Lexer(const std::string &src);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos{0};
    int line{1}, col{1};

    bool eof() const;
    char peek() const;
    char peekN(size_t n) const;
    char get();
    void skipWhitespace();
    Token makeToken(TokenType t, const std::string &v, int tl, int tc) const;

    static const std::unordered_map<std::string, TokenType> keywords;
    static const std::unordered_map<std::string, TokenType> twoCharOps;
    static const std::unordered_map<char, TokenType> singleCharOps;
    static const std::unordered_map<char, TokenType> punctuation;
};
