/*
 * ELF format support
 *
 * Copyright (c) 2019 Fernando Lemos
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "cutils.h"

typedef struct __attribute__((packed)) {
    uint8_t magic[4];
    uint8_t class;
    uint8_t data;
    uint8_t version;
    uint8_t osabi;
    uint8_t abiversion;
    uint8_t pad[7];
} elf_ident_t;

typedef struct __attribute__((packed)) {
    elf_ident_t ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header32_t;

typedef struct __attribute__((packed)) {
    elf_ident_t ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header64_t;

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} elf_pheader32_t;

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
} elf_pheader64_t;

#define PT_LOAD 0x01

int elf_detect_magic(const uint8_t *buf, int buf_len)
{
    if (buf_len < sizeof(elf_ident_t)) return 0;

    elf_ident_t *ident = (void *)buf;
    return (ident->magic[0] == 0x7f &&
            ident->magic[1] == 'E' &&
            ident->magic[2] == 'L' &&
            ident->magic[3] == 'F' &&
            ident->version == 1);
}

int elf_load(const uint8_t *inbuf, int inbuf_len,
             uint8_t *outbuf, int outbuf_len)
{
#define is_elf_64 (ident->class == 2)
#define is_big_endian (ident->data == 2)

#ifdef WORDS_BIGENDIAN
#define elf_to_cpu16(x) (is_big_endian ? (x) : bswap_16(x))
#define elf_to_cpu32(x) (is_big_endian ? (x) : bswap_32(x))
#define elf_to_cpu64(x) (is_big_endian ? (x) : bswap_64(x))
#else
#define elf_to_cpu16(x) (is_big_endian ? bswap_16(x) : (x))
#define elf_to_cpu32(x) (is_big_endian ? bswap_32(x) : (x))
#define elf_to_cpu64(x) (is_big_endian ? bswap_64(x) : (x))
#endif

#define header_field(bs, x) \
    (is_elf_64 ? \
        elf_to_cpu##bs(((elf_header64_t *)inbuf)->x) : \
        elf_to_cpu##bs(((elf_header32_t *)inbuf)->x))

#define header_dword(x) \
    (is_elf_64 ? \
        elf_to_cpu64(((elf_header64_t *)inbuf)->x) : \
        (uint64_t)elf_to_cpu32(((elf_header32_t *)inbuf)->x))

#define pheader_field(bs, x) \
    (is_elf_64 ? \
        elf_to_cpu##bs(((elf_pheader64_t *)(inbuf + phbase))->x) : \
        elf_to_cpu##bs(((elf_pheader32_t *)(inbuf + phbase))->x))

#define pheader_dword(x) \
    (is_elf_64 ? \
        elf_to_cpu64(((elf_pheader64_t *)(inbuf + phbase))->x) : \
        (uint64_t)elf_to_cpu32(((elf_pheader32_t *)(inbuf + phbase))->x))

    elf_ident_t *ident = (elf_ident_t *)inbuf;

    const uint16_t phnum = header_field(16, phnum);
    const uint16_t phentsize = header_field(16, phentsize);
    const uintptr_t phoff = (uintptr_t)header_dword(phoff);

    uintptr_t base_address = UINTPTR_MAX;
    uint16_t i;
    for (i = 0; i < phnum; i++) {
        const uintptr_t phbase = phoff + i * phentsize;
        const uint32_t type = pheader_field(32, type);
        if (type == PT_LOAD) {
            const uintptr_t paddr = (uintptr_t)pheader_dword(paddr);
            if (paddr < base_address) base_address = paddr;
        }
    }

    for (i = 0; i < phnum; i++) {
        const uintptr_t phbase = phoff + i * phentsize;
        const uint32_t type = pheader_field(32, type);
        if (type == PT_LOAD) {
            const uintptr_t offset = (uintptr_t)pheader_dword(offset);
            const uintptr_t paddr = (uintptr_t)pheader_dword(paddr);
            const size_t filesz = (size_t)pheader_dword(filesz);
            if (paddr - base_address + filesz > outbuf_len) return -1;
            memcpy(&outbuf[paddr - base_address], &inbuf[offset], filesz);
        }
    }

    return 0;
}
