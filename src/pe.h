/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#ifndef JAJACC_PE_H
#define JAJACC_PE_H

#include <ostream>

#include "codegen.h"

/* ImageBase 固定 0x400000，入口 RVA 0x1000 */
void writePE32(std::ostream &os, const CodeResult &res, uint32_t entryRVA);

#endif /* JAJACC_PE_H */
