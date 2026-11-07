/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#ifndef JAJACC_LEXER_H
#define JAJACC_LEXER_H

#include <cstddef>
#include <string>
#include <vector>

enum TokenType {
    TOK_EOF,
    TOK_INT, TOK_CHAR, TOK_VOID,
    TOK_RETURN, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_BREAK, TOK_CONTINUE,
    TOK_IDENT, TOK_NUM, TOK_STR, TOK_CHARLIT,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_ASSIGN, TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_LAND, TOK_LOR, TOK_NOT,
    TOK_INC, TOK_DEC, TOK_ADDEQ, TOK_SUBEQ, TOK_MULEQ, TOK_DIVEQ,
    TOK_SEMI, TOK_COMMA, TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET
};

struct Token {
    TokenType type;
    std::string text;
    long value;
    char ch;
    Token() : type(TOK_EOF), value(0), ch(0) {}
};

class Lexer {
public:
    void lex(const std::string &src);
    Token peek() const;
    Token consume();
    bool check(TokenType t) const;
    void expect(TokenType t);
private:
    std::vector<Token> toks_;
    size_t pos_ = 0;
};

#endif /* JAJACC_LEXER_H */
