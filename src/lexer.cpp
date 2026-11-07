/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#include "lexer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

static bool isIdStart(char c) {
    return std::isalpha((unsigned char)c) != 0 || c == '_';
}
static bool isIdChar(char c) {
    return std::isalnum((unsigned char)c) != 0 || c == '_';
}

void Lexer::lex(const std::string &src) {
    toks_.clear();
    pos_ = 0;

    size_t i = 0;
    while (i < src.size()) {
        unsigned char c = (unsigned char)src[i];

        if (std::isspace(c)) { ++i; continue; }

        /* // 单行注释 */
        if (c == '/' && i + 1 < src.size() && src[i + 1] == '/') {
            while (i < src.size() && src[i] != '\n') ++i;
            continue;
        }
        /* 块注释 */
        if (c == '/' && i + 1 < src.size() && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < src.size() && !(src[i] == '*' && src[i + 1] == '/')) ++i;
            if (i + 1 < src.size()) i += 2;
            else i = src.size();
            continue;
        }

        Token t;

        /* 标识符 / 关键字 */
        if (isIdStart((char)c)) {
            size_t st = i;
            while (i < src.size() && isIdChar(src[i])) ++i;
            std::string w = src.substr(st, i - st);

            if (w == "int")            t.type = TOK_INT;
            else if (w == "char")      t.type = TOK_CHAR;
            else if (w == "void")      t.type = TOK_VOID;
            else if (w == "return")    t.type = TOK_RETURN;
            else if (w == "if")        t.type = TOK_IF;
            else if (w == "else")      t.type = TOK_ELSE;
            else if (w == "while")     t.type = TOK_WHILE;
            else if (w == "for")       t.type = TOK_FOR;
            else if (w == "break")     t.type = TOK_BREAK;
            else if (w == "continue")  t.type = TOK_CONTINUE;
            else { t.type = TOK_IDENT; t.text = w; }

            toks_.push_back(t);
            continue;
        }

        /* 数字 */
        if (std::isdigit(c)) {
            long v = 0;
            size_t st = i;
            while (i < src.size() && std::isdigit((unsigned char)src[i])) {
                v = v * 10 + (src[i] - '0');
                ++i;
            }
            t.type = TOK_NUM;
            t.value = v;
            t.text = src.substr(st, i - st);
            toks_.push_back(t);
            continue;
        }

        /* 字符串字面量 */
        if (c == '"') {
            ++i;
            std::string s;
            while (i < src.size() && src[i] != '"') {
                if (src[i] == '\\' && i + 1 < src.size()) {
                    ++i;
                    if (src[i] == 'n') s += '\n';
                    else if (src[i] == 't') s += '\t';
                    else if (src[i] == 'r') s += '\r';
                    else if (src[i] == '0') s += '\0';
                    else s += src[i];
                    ++i;
                    continue;
                }
                s += src[i];
                ++i;
            }
            if (i < src.size()) ++i;
            t.type = TOK_STR;
            t.text = s;
            toks_.push_back(t);
            continue;
        }

        /* 字符字面量 */
        if (c == '\'') {
            ++i;
            char v = 0;
            if (i < src.size()) {
                if (src[i] == '\\' && i + 1 < src.size()) {
                    ++i;
                    if (src[i] == 'n') v = '\n';
                    else if (src[i] == 't') v = '\t';
                    else if (src[i] == '0') v = '\0';
                    else v = src[i];
                    ++i;
                } else {
                    v = src[i];
                    ++i;
                }
            }
            if (i < src.size() && src[i] == '\'') ++i;
            t.type = TOK_CHARLIT;
            t.ch = v;
            toks_.push_back(t);
            continue;
        }

        /* 运算符：先匹配两字符 */
        char c1 = (i + 1 < src.size()) ? src[i + 1] : '\0';
        t.type = TOK_EOF;
        bool two = true;

        if (c == '=' && c1 == '=')      t.type = TOK_EQ;
        else if (c == '!' && c1 == '=') t.type = TOK_NE;
        else if (c == '<' && c1 == '=') t.type = TOK_LE;
        else if (c == '>' && c1 == '=') t.type = TOK_GE;
        else if (c == '&' && c1 == '&') t.type = TOK_LAND;
        else if (c == '|' && c1 == '|') t.type = TOK_LOR;
        else if (c == '+' && c1 == '+') t.type = TOK_INC;
        else if (c == '-' && c1 == '-') t.type = TOK_DEC;
        else if (c == '+' && c1 == '=') t.type = TOK_ADDEQ;
        else if (c == '-' && c1 == '=') t.type = TOK_SUBEQ;
        else if (c == '*' && c1 == '=') t.type = TOK_MULEQ;
        else if (c == '/' && c1 == '=') t.type = TOK_DIVEQ;
        else two = false;

        if (two) {
            i += 2;
            toks_.push_back(t);
            continue;
        }

        bool one = true;
        if (c == '=')      t.type = TOK_ASSIGN;
        else if (c == '<') t.type = TOK_LT;
        else if (c == '>') t.type = TOK_GT;
        else if (c == '+') t.type = TOK_PLUS;
        else if (c == '-') t.type = TOK_MINUS;
        else if (c == '*') t.type = TOK_STAR;
        else if (c == '/') t.type = TOK_SLASH;
        else if (c == '%') t.type = TOK_PERCENT;
        else if (c == '!') t.type = TOK_NOT;
        else if (c == ';') t.type = TOK_SEMI;
        else if (c == ',') t.type = TOK_COMMA;
        else if (c == '(') t.type = TOK_LPAREN;
        else if (c == ')') t.type = TOK_RPAREN;
        else if (c == '{') t.type = TOK_LBRACE;
        else if (c == '}') t.type = TOK_RBRACE;
        else if (c == '[') t.type = TOK_LBRACKET;
        else if (c == ']') t.type = TOK_RBRACKET;
        else one = false;

        if (!one) {
            std::fprintf(stderr, "[JajaCC] lexer: unexpected character '%c'\n", src[i]);
            std::exit(1);
        }

        ++i;
        toks_.push_back(t);
    }

    Token eof;
    eof.type = TOK_EOF;
    toks_.push_back(eof);
}

Token Lexer::peek() const {
    if (pos_ >= toks_.size()) {
        Token t;
        t.type = TOK_EOF;
        return t;
    }
    return toks_[pos_];
}

Token Lexer::consume() {
    Token t = peek();
    if (pos_ < toks_.size()) ++pos_;
    return t;
}

bool Lexer::check(TokenType t) const {
    return peek().type == t;
}

void Lexer::expect(TokenType t) {
    if (peek().type != t) {
        std::fprintf(stderr, "[JajaCC] syntax error: unexpected token near position %lu\n",
                     (unsigned long)pos_);
        std::exit(1);
    }
    consume();
}
