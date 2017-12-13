/* Copyright (C) 1995-2011, 2016 Mark Adler
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Authors: Adenilson Cavalcanti <adenilson.cavalcanti@arm.com>
 *          Yang Zhang <yang.zhang@arm.com>
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *  claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include "armv8_crc32.h"
#include <arm_acle.h>

unsigned long armv8_crc32_little(unsigned long crc,
                                 const unsigned char *buf,
                                 size_t len)
{
    /* Same behavior as crc32_z(). */
    if (!buf)
        return 0UL;

    const uint64_t *buf4;
    uint32_t c = ~crc;

    while (len && ((uintptr_t)buf & 7)) {
        c = __crc32b(c, *buf++);
        --len;
    }

    buf4 = (const uint64_t *)buf;

    while (len >= 64) {
        c = __crc32d(c, *buf4++);
        c = __crc32d(c, *buf4++);
        c = __crc32d(c, *buf4++);
        c = __crc32d(c, *buf4++);

        c = __crc32d(c, *buf4++);
        c = __crc32d(c, *buf4++);
        c = __crc32d(c, *buf4++);
        c = __crc32d(c, *buf4++);
        len -= 64;
    }

    while (len >= 8) {
        c = __crc32d(c, *buf4++);
        len -= 8;
    }

    buf = (const unsigned char *)buf4;
    while (--len) {
        c = __crc32b(c, *buf++);
    }

    c = ~c;
    return c;
}
