/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#ifndef JAJACC_PARSER_H
#define JAJACC_PARSER_H

#include "ast.h"
#include "lexer.h"

class Parser {
public:
    NodePtr parseProgram(const std::string &src);
private:
    NodePtr parseFunction();
    NodePtr parseStatement();
    NodePtr parseBlock();
    NodePtr parseDeclaration(bool expectSemi);
    NodePtr parseExpr();
    NodePtr parseAssign();
    NodePtr parseLogicalOr();
    NodePtr parseLogicalAnd();
    NodePtr parseEquality();
    NodePtr parseComparison();
    NodePtr parseAdditive();
    NodePtr parseMultiplicative();
    NodePtr parseUnary();
    NodePtr parsePostfix();
    NodePtr parsePrimary();

    Lexer lx_;
};

#endif /* JAJACC_PARSER_H */
