/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#include "codegen.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

namespace {

const uint32_t IMAGE_BASE = 0x400000u;
const uint32_t RDATA_RVA  = 0x2000u;
const uint32_t EXIT_IAT   = 0x403030u; /* ImageBase + .idata RVA + IAT offset */

class Cg {
public:
    CodeResult run(Node *program);

private:
    std::vector<uint8_t> code_;
    std::vector<uint8_t> rdata_;
    std::map<std::string, int> vars_;
    int nlocals_ = 0;
    int epilogue_ = -1;

    std::vector<int> labels_;
    struct Fix { size_t at; int label; };
    std::vector<Fix> fixes_;

    std::vector<int> breakStack_;
    std::vector<int> continueStack_;

    /* ---- 标签 ---- */
    int newLabel() { labels_.push_back(-1); return (int)labels_.size() - 1; }
    void bindLabel(int l) { labels_[l] = (int)code_.size(); }
    void addFix(size_t at, int l) { Fix f; f.at = at; f.label = l; fixes_.push_back(f); }

    void resolveFixups() {
        for (size_t i = 0; i < fixes_.size(); ++i) {
            int target = labels_[fixes_[i].label];
            int rel = target - (int)(fixes_[i].at + 4);
            code_[fixes_[i].at + 0] = (uint8_t)((uint32_t)rel & 0xFF);
            code_[fixes_[i].at + 1] = (uint8_t)(((uint32_t)rel >> 8) & 0xFF);
            code_[fixes_[i].at + 2] = (uint8_t)(((uint32_t)rel >> 16) & 0xFF);
            code_[fixes_[i].at + 3] = (uint8_t)(((uint32_t)rel >> 24) & 0xFF);
        }
    }

    /* ---- 字节发射 ---- */
    void b(uint8_t v) { code_.push_back(v); }
    void d(uint32_t v) {
        b((uint8_t)(v & 0xFF));
        b((uint8_t)((v >> 8) & 0xFF));
        b((uint8_t)((v >> 16) & 0xFF));
        b((uint8_t)((v >> 24) & 0xFF));
    }

    /* ---- 指令 ---- */
    void iPushEbp()  { b(0x55); }
    void iMovEbpEsp() { b(0x89); b(0xE5); }
    void iMovEspEbp() { b(0x89); b(0xEC); }
    void iPopEbp()   { b(0x5D); }
    void iPushEax()  { b(0x50); }
    void iPopEbx()   { b(0x5B); }
    void iXchgEaxEbx() { b(0x93); }
    void iMovEaxImm(uint32_t v) { b(0xB8); d(v); }
    void iLoadLocal(int off) { b(0x8B); b(0x85); d((uint32_t)off); }
    void iStoreLocal(int off) { b(0x89); b(0x85); d((uint32_t)off); }
    void iSubEsp(uint32_t n) { b(0x81); b(0xEC); d(n); }
    void iTestEax() { b(0x85); b(0xC0); }
    void iAddEaxEbx() { b(0x01); b(0xD8); }
    void iAndEaxEbx() { b(0x21); b(0xD8); }
    void iOrEaxEbx() { b(0x09); b(0xD8); }
    void iSubEaxEbx() { b(0x29); b(0xD8); }
    void iImulEaxEbx() { b(0x0F); b(0xAF); b(0xC3); }
    void iNegEax() { b(0xF7); b(0xD8); }
    void iCdq() { b(0x99); }
    void iIdivEbx() { b(0xF7); b(0xFB); }
    void iMovEaxEdx() { b(0x89); b(0xD0); }
    void iCmpEaxEbx() { b(0x39); b(0xD8); }
    void iSetcc(uint8_t op) { b(0x0F); b(op); b(0xC0); }
    void iMovzxEaxAl() { b(0x0F); b(0xB6); b(0xC0); }
    void iIncEax() { b(0x40); }
    void iDecEax() { b(0x48); }
    void iXorEaxEax() { b(0x31); b(0xC0); }
    void iRet() { b(0xC3); }

    void iJmp(int l) { b(0xE9); size_t at = code_.size(); d(0); addFix(at, l); }
    void iJz(int l)  { b(0x0F); b(0x84); size_t at = code_.size(); d(0); addFix(at, l); }
    void iJnz(int l) { b(0x0F); b(0x85); size_t at = code_.size(); d(0); addFix(at, l); }
    void iCallAbs(uint32_t addr) { b(0xFF); b(0x15); d(addr); }

    void boolify() { iTestEax(); iSetcc(0x95); iMovzxEaxAl(); } /* eax = (eax != 0) */
    void boolNot() { iTestEax(); iSetcc(0x94); iMovzxEaxAl(); } /* eax = (eax == 0)，即 !eax */

    uint32_t addString(const std::string &s) {
        uint32_t off = (uint32_t)rdata_.size();
        for (size_t i = 0; i < s.size(); ++i) rdata_.push_back((uint8_t)s[i]);
        rdata_.push_back(0);
        while (rdata_.size() % 4 != 0) rdata_.push_back(0);
        return off;
    }

    void genExpr(Node *n);
    void genStmt(Node *n);
    void genBlock(Node *n);
    void genFunc(Node *n);
};

void Cg::genExpr(Node *n) {
    if (!n) return;

    switch (n->kind) {
    case N_NUM:
        iMovEaxImm((uint32_t)n->ival);
        break;

    case N_CHAR:
        iMovEaxImm((uint32_t)(unsigned char)n->cval);
        break;

    case N_STR: {
        uint32_t off = addString(n->name);
        iMovEaxImm(IMAGE_BASE + RDATA_RVA + off);
        break;
    }

    case N_VAR: {
        std::map<std::string, int>::iterator it = vars_.find(n->name);
        if (it == vars_.end()) {
            std::fprintf(stderr, "[JajaCC] semantic error: undeclared variable '%s'\n",
                         n->name.c_str());
            std::exit(1);
        }
        iLoadLocal(it->second);
        break;
    }

    case N_BIN: {
        /* 逻辑与 / 或：各自转成 0-1 再按位与/或 */
        if (n->op == TOK_LAND || n->op == TOK_LOR) {
            genExpr(n->lhs.get());
            boolify();
            iPushEax();
            genExpr(n->rhs.get());
            boolify();
            iPopEbx();
            if (n->op == TOK_LAND) iAndEaxEbx();
            else iOrEaxEbx();
            break;
        }

        genExpr(n->lhs.get());
        iPushEax();
        genExpr(n->rhs.get());
        iPopEbx();

        switch (n->op) {
        case TOK_PLUS:  iAddEaxEbx(); break;
        case TOK_MINUS: iXchgEaxEbx(); iSubEaxEbx(); break;
        case TOK_STAR:  iImulEaxEbx(); break;
        case TOK_SLASH: iXchgEaxEbx(); iCdq(); iIdivEbx(); break;
        case TOK_PERCENT: iXchgEaxEbx(); iCdq(); iIdivEbx(); iMovEaxEdx(); break;
        case TOK_EQ:  iXchgEaxEbx(); iCmpEaxEbx(); iSetcc(0x94); iMovzxEaxAl(); break;
        case TOK_NE:  iXchgEaxEbx(); iCmpEaxEbx(); iSetcc(0x95); iMovzxEaxAl(); break;
        case TOK_LT:  iXchgEaxEbx(); iCmpEaxEbx(); iSetcc(0x9C); iMovzxEaxAl(); break;
        case TOK_GT:  iXchgEaxEbx(); iCmpEaxEbx(); iSetcc(0x9F); iMovzxEaxAl(); break;
        case TOK_LE:  iXchgEaxEbx(); iCmpEaxEbx(); iSetcc(0x9E); iMovzxEaxAl(); break;
        case TOK_GE:  iXchgEaxEbx(); iCmpEaxEbx(); iSetcc(0x9D); iMovzxEaxAl(); break;
        default: break;
        }
        break;
    }

    case N_UNARY: {
        if (n->op == TOK_INC || n->op == TOK_DEC) {
            Node *op = n->lhs.get();
            if (op && op->kind == N_VAR) {
                std::map<std::string, int>::iterator it = vars_.find(op->name);
                if (it == vars_.end()) {
                    std::fprintf(stderr, "[JajaCC] semantic error: undeclared variable '%s'\n",
                                 op->name.c_str());
                    std::exit(1);
                }
                iLoadLocal(it->second);
                if (n->op == TOK_INC) iIncEax(); else iDecEax();
                iStoreLocal(it->second);
                break;
            }
        }
        genExpr(n->lhs.get());
        if (n->op == TOK_MINUS) iNegEax();
        else if (n->op == TOK_NOT) boolNot();
        break;
    }

    case N_ASSIGN: {
        Node *lhs = n->lhs.get();
        if (!lhs || lhs->kind != N_VAR) {
            std::fprintf(stderr, "[JajaCC] semantic error: assignment target must be a variable\n");
            std::exit(1);
        }
        std::map<std::string, int>::iterator it = vars_.find(lhs->name);
        if (it == vars_.end()) {
            std::fprintf(stderr, "[JajaCC] semantic error: undeclared variable '%s'\n",
                         lhs->name.c_str());
            std::exit(1);
        }
        int off = it->second;

        if (n->op == TOK_ASSIGN) {
            genExpr(n->rhs.get());
        } else {
            iLoadLocal(off);
            iPushEax();
            genExpr(n->rhs.get());
            iPopEbx();
            if (n->op == TOK_ADDEQ) iAddEaxEbx();
            else if (n->op == TOK_SUBEQ) { iXchgEaxEbx(); iSubEaxEbx(); }
            else if (n->op == TOK_MULEQ) iImulEaxEbx();
            else if (n->op == TOK_DIVEQ) { iXchgEaxEbx(); iCdq(); iIdivEbx(); }
        }
        iStoreLocal(off);
        break;
    }

    case N_CALL: {
        /* 目前只做常量求值占位：返回 0（见 README） */
        for (size_t k = 0; k < n->items.size(); ++k) {
            genExpr(n->items[k].get());
        }
        iXorEaxEax();
        break;
    }

    default:
        iXorEaxEax();
        break;
    }
}

void Cg::genStmt(Node *n) {
    if (!n) return;

    switch (n->kind) {
    case N_BLOCK:
        genBlock(n);
        break;

    case N_DECL: {
        nlocals_ += 4;
        int off = -nlocals_;
        iSubEsp(4);
        vars_[n->name] = off;
        if (n->lhs) genExpr(n->lhs.get());
        else iMovEaxImm(0);
        iStoreLocal(off);
        break;
    }

    case N_IF: {
        int lElse = newLabel();
        int lEnd  = newLabel();
        genExpr(n->lhs.get());
        iTestEax();
        iJz(lElse);
        genStmt(n->rhs.get());
        if (n->els) {
            iJmp(lEnd);
            bindLabel(lElse);
            genStmt(n->els.get());
        } else {
            /* 无 else：跳过 then，直接落到 lElse 之后 */
            bindLabel(lElse);
        }
        bindLabel(lEnd);
        break;
    }

    case N_WHILE: {
        int lCond = newLabel();
        int lEnd  = newLabel();
        bindLabel(lCond);
        genExpr(n->lhs.get());
        iTestEax();
        iJz(lEnd);
        breakStack_.push_back(lEnd);
        continueStack_.push_back(lCond);
        genStmt(n->body.get());
        breakStack_.pop_back();
        continueStack_.pop_back();
        iJmp(lCond);
        bindLabel(lEnd);
        break;
    }

    case N_FOR: {
        int lCond = newLabel();
        int lStep = newLabel();
        int lEnd  = newLabel();

        if (n->init) genStmt(n->init.get());

        bindLabel(lCond);
        if (n->lhs) {
            genExpr(n->lhs.get());
            iTestEax();
            iJz(lEnd);
        }

        breakStack_.push_back(lEnd);
        continueStack_.push_back(lStep);
        genStmt(n->body.get());
        breakStack_.pop_back();
        continueStack_.pop_back();

        bindLabel(lStep);
        if (n->step) genExpr(n->step.get());
        iJmp(lCond);
        bindLabel(lEnd);
        break;
    }

    case N_RETURN:
        if (n->lhs) genExpr(n->lhs.get());
        else iMovEaxImm(0);
        iJmp(epilogue_);
        break;

    case N_BREAK:
        if (!breakStack_.empty()) iJmp(breakStack_.back());
        break;

    case N_CONTINUE:
        if (!continueStack_.empty()) iJmp(continueStack_.back());
        break;

    default:
        genExpr(n);
        break;
    }
}

void Cg::genBlock(Node *n) {
    for (size_t i = 0; i < n->items.size(); ++i) {
        genStmt(n->items[i].get());
    }
}

void Cg::genFunc(Node *n) {
    epilogue_ = newLabel();

    iPushEbp();
    iMovEbpEsp();
    iMovEaxImm(0);

    genStmt(n->body.get());

    bindLabel(epilogue_);
    iMovEspEbp();
    iPopEbp();
    iPushEax();                 /* 返回值作为退出码 */
    iCallAbs(EXIT_IAT);         /* call dword [ExitProcess] */
    iRet();
}

CodeResult Cg::run(Node *program) {
    Node *target = 0;
    for (size_t i = 0; i < program->items.size(); ++i) {
        Node *f = program->items[i].get();
        if (f->kind == N_FUNC) {
            if (f->name == "main") { target = f; break; }
            if (!target) target = f;
        }
    }
    if (!target) {
        std::fprintf(stderr, "[JajaCC] semantic error: no function found\n");
        std::exit(1);
    }

    genFunc(target);
    resolveFixups();

    CodeResult r;
    r.text = code_;
    r.rdata = rdata_;
    return r;
}

} /* namespace */

CodeResult generate(NodePtr program) {
    Cg cg;
    return cg.run(program.get());
}
