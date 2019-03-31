/*
 * Compressed file support
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

#include <inttypes.h>
#include <stdio.h>
#include <zlib.h>

#include "cutils.h"

typedef enum {
    ALGORITHM_UNKNOWN,
    ALGORITHM_DEFLATE,
    ALGORITHM_GZIP,
} algorithm_t;

static algorithm_t detect_algorithm(const uint8_t *buf, int len)
{
    if (len < 2 || buf[0] != 0x1f) return ALGORITHM_UNKNOWN;

    switch (buf[1]) {
        case 0x08: return ALGORITHM_DEFLATE;
        case 0x8b: return ALGORITHM_GZIP;
        default: return ALGORITHM_UNKNOWN;
    }
}

int compress_detect_magic(const uint8_t *buf, int len)
{
    return detect_algorithm(buf, len) != ALGORITHM_UNKNOWN;
}

int decompress(const uint8_t *in_buf, int in_len,
               const uint8_t *out_buf, int out_len)
{
    int windowBits;
    switch (detect_algorithm(in_buf, in_len)) {
        case ALGORITHM_UNKNOWN:
            return -1;
        case ALGORITHM_DEFLATE:
            windowBits = -MAX_WBITS;
            break;
        case ALGORITHM_GZIP:
            windowBits = MAX_WBITS | 16;
            break;
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    if (inflateInit2(&strm, windowBits) != Z_OK) return -1;

    strm.avail_in = in_len;
    strm.next_in = (Bytef *)in_buf;
    strm.avail_out = out_len;
    strm.next_out = (Bytef *)out_buf;
    if (inflate(&strm, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "inflate() failed: %s\n", strm.msg);
        inflateEnd(&strm);
        return -1;
    }

    return strm.total_out;
}
