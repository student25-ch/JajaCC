/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "codegen.h"
#include "parser.h"
#include "pe.h"

static void usage(const char *self) {
    std::fprintf(stderr,
        "JajaCC - Standalone C Compiler (GPLv3)\n"
        "Copyright (C) 2026 student25-ch\n"
        "\n"
        "usage: %s <input.c> [output.exe]\n"
        "\n"
        "  input.c     C source file\n"
        "  output.exe  output Windows PE (default: a.exe)\n",
        self);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::string inPath = argv[1];
    std::string outPath = (argc >= 3) ? argv[2] : "a.exe";

    std::ifstream fin(inPath.c_str(), std::ios::binary);
    if (!fin) {
        std::fprintf(stderr, "[JajaCC] error: cannot open '%s'\n", inPath.c_str());
        return 1;
    }
    std::stringstream ss;
    ss << fin.rdbuf();
    std::string src = ss.str();
    fin.close();

    std::printf("[JajaCC] parsing %s ...\n", inPath.c_str());
    NodePtr ast = Parser().parseProgram(src);

    std::printf("[JajaCC] generating code ...\n");
    CodeResult code = generate(std::move(ast));

    std::ofstream fout(outPath.c_str(), std::ios::binary);
    if (!fout) {
        std::fprintf(stderr, "[JajaCC] error: cannot write '%s'\n", outPath.c_str());
        return 1;
    }
    writePE32(fout, code, 0x1000u);
    fout.close();

    std::printf("[JajaCC] done -> %s (%lu bytes of .text)\n",
                outPath.c_str(), (unsigned long)code.text.size());
    return 0;
}
