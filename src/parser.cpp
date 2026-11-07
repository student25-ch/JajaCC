/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#include "parser.h"

#include <cstdio>
#include <cstdlib>

static bool isTypeToken(TokenType t) {
    return t == TOK_INT || t == TOK_CHAR || t == TOK_VOID;
}

NodePtr Parser::parseProgram(const std::string &src) {
    lx_.lex(src);

    NodePtr prog(new Node(N_PROGRAM));
    while (!lx_.check(TOK_EOF)) {
        NodePtr f = parseFunction();
        if (!f) break;
        prog->items.push_back(std::move(f));
    }
    return prog;
}

NodePtr Parser::parseFunction() {
    if (!isTypeToken(lx_.peek().type)) {
        std::fprintf(stderr, "[JajaCC] syntax error: expected type at top level\n");
        std::exit(1);
    }
    lx_.consume();

    Token nameTok = lx_.consume();
    if (nameTok.type != TOK_IDENT) {
        std::fprintf(stderr, "[JajaCC] syntax error: expected function name\n");
        std::exit(1);
    }

    lx_.expect(TOK_LPAREN);

    /* 参数列表：解析但暂不使用（见 README 的“暂未支持”） */
    if (!lx_.check(TOK_RPAREN)) {
        for (;;) {
            if (isTypeToken(lx_.peek().type)) lx_.consume();
            if (lx_.check(TOK_IDENT)) lx_.consume();
            if (lx_.check(TOK_COMMA)) { lx_.consume(); continue; }
            break;
        }
    }
    lx_.expect(TOK_RPAREN);

    NodePtr body = parseBlock();

    NodePtr fn(new Node(N_FUNC));
    fn->name = nameTok.text;
    fn->body = std::move(body);
    return fn;
}

NodePtr Parser::parseBlock() {
    lx_.expect(TOK_LBRACE);

    NodePtr blk(new Node(N_BLOCK));
    while (!lx_.check(TOK_RBRACE) && !lx_.check(TOK_EOF)) {
        blk->items.push_back(parseStatement());
    }
    lx_.expect(TOK_RBRACE);
    return blk;
}

NodePtr Parser::parseDeclaration(bool expectSemi) {
    lx_.consume(); /* type */

    NodePtr d(new Node(N_DECL));
    Token nm = lx_.consume();
    if (nm.type != TOK_IDENT) {
        std::fprintf(stderr, "[JajaCC] syntax error: expected variable name\n");
        std::exit(1);
    }
    d->name = nm.text;

    if (lx_.check(TOK_ASSIGN)) {
        lx_.consume();
        d->lhs = parseExpr();
    }

    if (expectSemi) lx_.expect(TOK_SEMI);
    return d;
}

NodePtr Parser::parseStatement() {
    Token t = lx_.peek();

    if (isTypeToken(t.type)) return parseDeclaration(true);

    if (t.type == TOK_LBRACE) return parseBlock();

    if (t.type == TOK_RETURN) {
        lx_.consume();
        NodePtr n(new Node(N_RETURN));
        if (!lx_.check(TOK_SEMI)) n->lhs = parseExpr();
        lx_.expect(TOK_SEMI);
        return n;
    }

    if (t.type == TOK_IF) {
        lx_.consume();
        lx_.expect(TOK_LPAREN);
        NodePtr cond = parseExpr();
        lx_.expect(TOK_RPAREN);
        NodePtr thenB = parseStatement();
        NodePtr n(new Node(N_IF));
        n->lhs = std::move(cond);
        n->rhs = std::move(thenB);
        if (lx_.check(TOK_ELSE)) {
            lx_.consume();
            n->els = parseStatement();
        }
        return n;
    }

    if (t.type == TOK_WHILE) {
        lx_.consume();
        lx_.expect(TOK_LPAREN);
        NodePtr cond = parseExpr();
        lx_.expect(TOK_RPAREN);
        NodePtr n(new Node(N_WHILE));
        n->lhs = std::move(cond);
        n->body = parseStatement();
        return n;
    }

    if (t.type == TOK_FOR) {
        lx_.consume();
        lx_.expect(TOK_LPAREN);

        NodePtr init;
        if (!lx_.check(TOK_SEMI)) {
            if (isTypeToken(lx_.peek().type)) init = parseDeclaration(false);
            else init = parseExpr();
        }
        lx_.expect(TOK_SEMI);

        NodePtr cond;
        if (!lx_.check(TOK_SEMI)) cond = parseExpr();
        lx_.expect(TOK_SEMI);

        NodePtr step;
        if (!lx_.check(TOK_RPAREN)) step = parseExpr();
        lx_.expect(TOK_RPAREN);

        NodePtr n(new Node(N_FOR));
        n->init = std::move(init);
        n->lhs  = std::move(cond);
        n->step = std::move(step);
        n->body = parseStatement();
        return n;
    }

    if (t.type == TOK_BREAK) {
        lx_.consume();
        lx_.expect(TOK_SEMI);
        return NodePtr(new Node(N_BREAK));
    }

    if (t.type == TOK_CONTINUE) {
        lx_.consume();
        lx_.expect(TOK_SEMI);
        return NodePtr(new Node(N_CONTINUE));
    }

    if (t.type == TOK_SEMI) {
        lx_.consume();
        return NodePtr(new Node(N_BLOCK)); /* 空语句 */
    }

    NodePtr e = parseExpr();
    lx_.expect(TOK_SEMI);
    return e;
}

NodePtr Parser::parseExpr() {
    return parseAssign();
}

NodePtr Parser::parseAssign() {
    NodePtr left = parseLogicalOr();

    TokenType t = lx_.peek().type;
    if (t == TOK_ASSIGN || t == TOK_ADDEQ || t == TOK_SUBEQ ||
        t == TOK_MULEQ || t == TOK_DIVEQ) {
        TokenType op = lx_.consume().type;
        NodePtr right = parseAssign();
        NodePtr n(new Node(N_ASSIGN));
        n->op = op;
        n->lhs = std::move(left);
        n->rhs = std::move(right);
        return n;
    }
    return left;
}

NodePtr Parser::parseLogicalOr() {
    NodePtr p = parseLogicalAnd();
    while (lx_.check(TOK_LOR)) {
        TokenType op = lx_.consume().type;
        NodePtr r = parseLogicalAnd();
        NodePtr n(new Node(N_BIN));
        n->op = op;
        n->lhs = std::move(p);
        n->rhs = std::move(r);
        p = std::move(n);
    }
    return p;
}

NodePtr Parser::parseLogicalAnd() {
    NodePtr p = parseEquality();
    while (lx_.check(TOK_LAND)) {
        TokenType op = lx_.consume().type;
        NodePtr r = parseEquality();
        NodePtr n(new Node(N_BIN));
        n->op = op;
        n->lhs = std::move(p);
        n->rhs = std::move(r);
        p = std::move(n);
    }
    return p;
}

NodePtr Parser::parseEquality() {
    NodePtr p = parseComparison();
    while (lx_.check(TOK_EQ) || lx_.check(TOK_NE)) {
        TokenType op = lx_.consume().type;
        NodePtr r = parseComparison();
        NodePtr n(new Node(N_BIN));
        n->op = op;
        n->lhs = std::move(p);
        n->rhs = std::move(r);
        p = std::move(n);
    }
    return p;
}

NodePtr Parser::parseComparison() {
    NodePtr p = parseAdditive();
    while (lx_.check(TOK_LT) || lx_.check(TOK_GT) ||
           lx_.check(TOK_LE) || lx_.check(TOK_GE)) {
        TokenType op = lx_.consume().type;
        NodePtr r = parseAdditive();
        NodePtr n(new Node(N_BIN));
        n->op = op;
        n->lhs = std::move(p);
        n->rhs = std::move(r);
        p = std::move(n);
    }
    return p;
}

NodePtr Parser::parseAdditive() {
    NodePtr p = parseMultiplicative();
    while (lx_.check(TOK_PLUS) || lx_.check(TOK_MINUS)) {
        TokenType op = lx_.consume().type;
        NodePtr r = parseMultiplicative();
        NodePtr n(new Node(N_BIN));
        n->op = op;
        n->lhs = std::move(p);
        n->rhs = std::move(r);
        p = std::move(n);
    }
    return p;
}

NodePtr Parser::parseMultiplicative() {
    NodePtr p = parseUnary();
    while (lx_.check(TOK_STAR) || lx_.check(TOK_SLASH) || lx_.check(TOK_PERCENT)) {
        TokenType op = lx_.consume().type;
        NodePtr r = parseUnary();
        NodePtr n(new Node(N_BIN));
        n->op = op;
        n->lhs = std::move(p);
        n->rhs = std::move(r);
        p = std::move(n);
    }
    return p;
}

NodePtr Parser::parseUnary() {
    TokenType t = lx_.peek().type;

    if (t == TOK_MINUS || t == TOK_NOT || t == TOK_PLUS ||
        t == TOK_INC || t == TOK_DEC) {
        TokenType op = lx_.consume().type;
        NodePtr operand = parseUnary();
        NodePtr n(new Node(N_UNARY));
        n->op = op;
        n->lhs = std::move(operand);
        return n;
    }
    return parsePostfix();
}

NodePtr Parser::parsePostfix() {
    NodePtr p = parsePrimary();

    /* 后缀 ++ / --：x++ 等价于 (x = x + 1)，这里直接复用自增语义 */
    while (lx_.check(TOK_INC) || lx_.check(TOK_DEC)) {
        TokenType op = lx_.consume().type;
        NodePtr n(new Node(N_UNARY));
        n->op = op;
        n->ival = 1; /* 标记为后缀形式 */
        n->lhs = std::move(p);
        p = std::move(n);
    }
    return p;
}

NodePtr Parser::parsePrimary() {
    Token t = lx_.peek();

    if (t.type == TOK_NUM) {
        lx_.consume();
        NodePtr n(new Node(N_NUM));
        n->ival = t.value;
        return n;
    }

    if (t.type == TOK_CHARLIT) {
        lx_.consume();
        NodePtr n(new Node(N_CHAR));
        n->cval = t.ch;
        return n;
    }

    if (t.type == TOK_STR) {
        lx_.consume();
        NodePtr n(new Node(N_STR));
        n->name = t.text;
        return n;
    }

    if (t.type == TOK_IDENT) {
        lx_.consume();

        if (lx_.check(TOK_LPAREN)) {
            lx_.consume();
            NodePtr call(new Node(N_CALL));
            call->name = t.text;
            if (!lx_.check(TOK_RPAREN)) {
                call->items.push_back(parseExpr());
                while (lx_.check(TOK_COMMA)) {
                    lx_.consume();
                    call->items.push_back(parseExpr());
                }
            }
            lx_.expect(TOK_RPAREN);
            return call;
        }

        NodePtr n(new Node(N_VAR));
        n->name = t.text;
        return n;
    }

    if (t.type == TOK_LPAREN) {
        lx_.consume();
        NodePtr e = parseExpr();
        lx_.expect(TOK_RPAREN);
        return e;
    }

    std::fprintf(stderr, "[JajaCC] syntax error: unexpected token in expression\n");
    std::exit(1);
}
