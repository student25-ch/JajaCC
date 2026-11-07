/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#ifndef JAJACC_CODEGEN_H
#define JAJACC_CODEGEN_H

#include <cstdint>
#include <vector>

#include "ast.h"

struct CodeResult {
    std::vector<uint8_t> text;
    std::vector<uint8_t> rdata;
};

CodeResult generate(NodePtr program);

#endif /* JAJACC_CODEGEN_H */
