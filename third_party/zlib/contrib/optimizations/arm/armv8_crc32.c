/* armv8_crc32.c -- CRC32 checksums using ARMv8-a crypto instructions.
 * Copyright (C) 1995-2011, 2016 Mark Adler
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Authors: Adenilson Cavalcanti <adenilson.cavalcanti@arm.com>
 *          Yang Zhang <yang.zhang@arm.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */
#include "armv8_crc32.h"
#include <arm_acle.h>

unsigned long armv8_crc32_little(unsigned long crc,
                                 const unsigned char *buf,
                                 size_t len)
{
    /* Same behavior as crc32_z(). */
    if (!len)
        return crc;

    const uint64_t *buf8;
    uint32_t c = ~crc;

    while (len && ((uintptr_t)buf & 7)) {
        c = __crc32b(c, *buf++);
        --len;
    }

    buf8 = (const uint64_t *)buf;

    while (len >= 64) {
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);

        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        len -= 64;
    }

    while (len >= 8) {
        c = __crc32d(c, *buf8++);
        len -= 8;
    }

    buf = (const unsigned char *)buf8;
    while (len--) {
        c = __crc32b(c, *buf++);
    }

    c = ~c;
    return c;
}
