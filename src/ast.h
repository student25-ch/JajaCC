/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#ifndef JAJACC_AST_H
#define JAJACC_AST_H

#include <memory>
#include <string>
#include <vector>

#include "lexer.h"

enum NodeKind {
    N_PROGRAM,
    N_FUNC,
    N_BLOCK,
    N_DECL,
    N_IF,
    N_WHILE,
    N_FOR,
    N_RETURN,
    N_BREAK,
    N_CONTINUE,
    N_NUM,
    N_CHAR,
    N_STR,
    N_VAR,
    N_BIN,
    N_UNARY,
    N_ASSIGN,
    N_CALL
};

struct Node;
typedef std::unique_ptr<Node> NodePtr;

struct Node {
    NodeKind kind;
    long ival;
    char cval;
    std::string name;
    TokenType op;

    NodePtr lhs;    /* 左值 / 条件 / 返回表达式 */
    NodePtr rhs;    /* 右值 / then 分支 */
    NodePtr els;    /* else 分支 */
    NodePtr init;   /* for 初始化 */
    NodePtr step;   /* for 步进 */
    NodePtr body;   /* 循环体 / 函数体 */

    std::vector<NodePtr> items; /* 语句列表 / 函数列表 / 实参列表 */

    explicit Node(NodeKind k)
        : kind(k), ival(0), cval(0), op(TOK_EOF) {}
};

#endif /* JAJACC_AST_H */
