/*
 * JajaCC - Standalone C Compiler
 * Copyright (C) 2026 student25-ch
 * Licensed under the GNU General Public License v3.
 */
#include "pe.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

const uint32_t IMAGE_BASE = 0x400000u;
const uint32_t SECT_ALIGN = 0x1000u;
const uint32_t FILE_ALIGN = 0x200u;
const uint32_t TEXT_RVA   = 0x1000u;
const uint32_t RDATA_RVA  = 0x2000u;
const uint32_t IDATA_RVA  = 0x3000u;
const uint32_t HDR_SIZE   = 0x400u;

uint32_t alignUp(uint32_t v, uint32_t a) {
    return ((v + a - 1) / a) * a;
}

void put8(std::vector<uint8_t> &v, uint8_t x) { v.push_back(x); }
void put16(std::vector<uint8_t> &v, uint16_t x) {
    put8(v, (uint8_t)(x & 0xFF));
    put8(v, (uint8_t)((x >> 8) & 0xFF));
}
void put32(std::vector<uint8_t> &v, uint32_t x) {
    put8(v, (uint8_t)(x & 0xFF));
    put8(v, (uint8_t)((x >> 8) & 0xFF));
    put8(v, (uint8_t)((x >> 16) & 0xFF));
    put8(v, (uint8_t)((x >> 24) & 0xFF));
}

/*
 * .idata 布局（相对 IDATA_RVA = 0x3000）
 *   0x00  Import Directory Table   (2 项 20 字节 = 40)
 *   0x28  ILT (OriginalFirstThunk) (2 个 dword = 8)
 *   0x30  IAT (FirstThunk)         (2 个 dword = 8)
 *   0x38  Hint/Name                (2 + "ExitProcess" + 0)
 *   0x48  DLL 名称                 ("KERNEL32.dll" + 0)
 */
std::vector<uint8_t> buildIdata() {
    std::vector<uint8_t> d;

    const uint32_t ILT = IDATA_RVA + 0x28u;
    const uint32_t IAT = IDATA_RVA + 0x30u;
    const uint32_t HIN = IDATA_RVA + 0x38u;
    const uint32_t DLL = IDATA_RVA + 0x48u;

    /* Import Directory Table */
    put32(d, ILT);   /* OriginalFirstThunk */
    put32(d, 0);     /* TimeDateStamp */
    put32(d, 0);     /* ForwarderChain */
    put32(d, DLL);   /* Name */
    put32(d, IAT);   /* FirstThunk */
    /* 结束项 */
    for (int i = 0; i < 5; ++i) put32(d, 0);

    /* ILT */
    put32(d, HIN);
    put32(d, 0);

    /* IAT（必须与 ILT 内容一致） */
    put32(d, HIN);
    put32(d, 0);

    /* Hint/Name */
    put16(d, 0);
    const char *nm = "ExitProcess";
    for (size_t i = 0; i < strlen(nm); ++i) put8(d, (uint8_t)nm[i]);
    put8(d, 0);
    while (d.size() < 0x48u) put8(d, 0);

    /* DLL 名称 */
    const char *dll = "KERNEL32.dll";
    for (size_t i = 0; i < strlen(dll); ++i) put8(d, (uint8_t)dll[i]);
    put8(d, 0);
    while (d.size() % 8 != 0) put8(d, 0);

    return d;
}

struct Sec {
    std::string name;
    uint32_t vsize;
    uint32_t rva;
    uint32_t rawSize;
    uint32_t rawPtr;
    uint32_t flags;
};

} /* namespace */

void writePE32(std::ostream &os, const CodeResult &res, uint32_t entryRVA) {
    std::vector<uint8_t> idata = buildIdata();

    std::vector<uint8_t> rdata = res.rdata;
    if (rdata.empty()) rdata.push_back(0);

    uint32_t textRawPtr  = HDR_SIZE;
    uint32_t rdataRawPtr = textRawPtr + alignUp((uint32_t)res.text.size(), FILE_ALIGN);
    uint32_t idataRawPtr = rdataRawPtr + alignUp((uint32_t)rdata.size(), FILE_ALIGN);

    Sec text;
    text.name = ".text";
    text.vsize = (uint32_t)res.text.size();
    text.rva = TEXT_RVA;
    text.rawSize = (uint32_t)res.text.size();
    text.rawPtr = textRawPtr;
    text.flags = 0x60000020u; /* code | execute | read */

    Sec rdataSec;
    rdataSec.name = ".rdata";
    rdataSec.vsize = (uint32_t)rdata.size();
    rdataSec.rva = RDATA_RVA;
    rdataSec.rawSize = (uint32_t)rdata.size();
    rdataSec.rawPtr = rdataRawPtr;
    rdataSec.flags = 0x40000040u; /* initialized | read */

    Sec idataSec;
    idataSec.name = ".idata";
    idataSec.vsize = (uint32_t)idata.size();
    idataSec.rva = IDATA_RVA;
    idataSec.rawSize = (uint32_t)idata.size();
    idataSec.rawPtr = idataRawPtr;
    idataSec.flags = 0xC0000040u; /* initialized | read | write */

    uint32_t sizeOfImage = alignUp(IDATA_RVA + (uint32_t)idata.size(), SECT_ALIGN);

    /* ---------------- DOS 头 + 跳板 ---------------- */
    std::vector<uint8_t> hdr;
    hdr.resize(0x80, 0);   /* DOS 头 + 跳板，PE 签名必须落在 e_lfanew=0x80 */
    hdr[0] = 'M';
    hdr[1] = 'Z';

    /* e_lfanew = 0x80 */
    hdr[0x3C] = 0x80;
    hdr[0x3D] = 0x00;
    hdr[0x3E] = 0x00;
    hdr[0x3F] = 0x00;

    /* ---------------- PE 签名 + COFF 头 ---------------- */
    put32(hdr, 0x00004550u);      /* "PE\0\0" */
    put16(hdr, 0x014Cu);          /* Machine = i386 */
    put16(hdr, 3);                /* NumberOfSections */
    put32(hdr, 0);                /* TimeDateStamp */
    put32(hdr, 0);                /* PointerToSymbolTable */
    put32(hdr, 0);                /* NumberOfSymbols */
    put16(hdr, 0x00E0u);          /* SizeOfOptionalHeader */
    put16(hdr, 0x0103u);          /* Characteristics: EXECUTABLE | RELOCS_STRIPPED | 32BIT */

    /* ---------------- Optional Header (PE32) ---------------- */
    put16(hdr, 0x010Bu);          /* Magic */
    put8(hdr, 9);                 /* MajorLinkerVersion */
    put8(hdr, 0);                 /* MinorLinkerVersion */
    put32(hdr, (uint32_t)res.text.size());                 /* SizeOfCode */
    put32(hdr, (uint32_t)(rdata.size() + idata.size()));   /* SizeOfInitializedData */
    put32(hdr, 0);                /* SizeOfUninitializedData */
    put32(hdr, entryRVA);         /* AddressOfEntryPoint */
    put32(hdr, TEXT_RVA);         /* BaseOfCode */
    put32(hdr, RDATA_RVA);        /* BaseOfData */
    put32(hdr, IMAGE_BASE);       /* ImageBase */
    put32(hdr, SECT_ALIGN);       /* SectionAlignment */
    put32(hdr, FILE_ALIGN);       /* FileAlignment */
    put16(hdr, 4);                /* MajorOperatingSystemVersion */
    put16(hdr, 0);                /* MinorOperatingSystemVersion */
    put16(hdr, 0);                /* MajorImageVersion */
    put16(hdr, 0);                /* MinorImageVersion */
    put16(hdr, 4);                /* MajorSubsystemVersion */
    put16(hdr, 0);                /* MinorSubsystemVersion */
    put32(hdr, 0);                /* Win32VersionValue */
    put32(hdr, sizeOfImage);      /* SizeOfImage */
    put32(hdr, HDR_SIZE);         /* SizeOfHeaders */
    put32(hdr, 0);                /* CheckSum */
    put16(hdr, 3);                /* Subsystem = console */
    put16(hdr, 0);                /* DllCharacteristics */
    put32(hdr, 0x100000u);        /* SizeOfStackReserve */
    put32(hdr, 0x1000u);          /* SizeOfStackCommit */
    put32(hdr, 0x100000u);        /* SizeOfHeapReserve */
    put32(hdr, 0x1000u);          /* SizeOfHeapCommit */
    put32(hdr, 0);                /* LoaderFlags */
    put32(hdr, 16);               /* NumberOfRvaAndSizes */

    /* DataDirectory[16] */
    put32(hdr, 0); put32(hdr, 0);                 /* 0  Export */
    put32(hdr, IDATA_RVA); put32(hdr, 40);        /* 1  Import */
    for (int i = 2; i < 16; ++i) { put32(hdr, 0); put32(hdr, 0); }

    /* ---------------- 节表 ---------------- */
    Sec *secs[3] = { &text, &rdataSec, &idataSec };
    for (int i = 0; i < 3; ++i) {
        Sec *s = secs[i];
        char nm[8];
        memset(nm, 0, sizeof(nm));
        memcpy(nm, s->name.c_str(), s->name.size() > 8 ? 8 : s->name.size());
        for (int k = 0; k < 8; ++k) put8(hdr, (uint8_t)nm[k]);
        put32(hdr, s->vsize);
        put32(hdr, s->rva);
        put32(hdr, alignUp(s->rawSize, FILE_ALIGN));
        put32(hdr, s->rawPtr);
        put32(hdr, 0);   /* PointerToRelocations */
        put32(hdr, 0);   /* PointerToLinenumbers */
        put16(hdr, 0);   /* NumberOfRelocations */
        put16(hdr, 0);   /* NumberOfLinenumbers */
        put32(hdr, s->flags);
    }

    /* 补齐到 HDR_SIZE */
    while (hdr.size() < HDR_SIZE) hdr.push_back(0);

    os.write((const char *)&hdr[0], (std::streamsize)hdr.size());

    /* ---------------- .text ---------------- */
    if (!res.text.empty())
        os.write((const char *)&res.text[0], (std::streamsize)res.text.size());
    {
        uint32_t pad = alignUp((uint32_t)res.text.size(), FILE_ALIGN) - (uint32_t)res.text.size();
        for (uint32_t i = 0; i < pad; ++i) os.put(0);
    }

    /* ---------------- .rdata ---------------- */
    os.write((const char *)&rdata[0], (std::streamsize)rdata.size());
    {
        uint32_t pad = alignUp((uint32_t)rdata.size(), FILE_ALIGN) - (uint32_t)rdata.size();
        for (uint32_t i = 0; i < pad; ++i) os.put(0);
    }

    /* ---------------- .idata ---------------- */
    os.write((const char *)&idata[0], (std::streamsize)idata.size());
    {
        uint32_t pad = alignUp((uint32_t)idata.size(), FILE_ALIGN) - (uint32_t)idata.size();
        for (uint32_t i = 0; i < pad; ++i) os.put(0);
    }
}
