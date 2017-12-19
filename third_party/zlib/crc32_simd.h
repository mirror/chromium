/* crc32_simd.h
 *
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */

#include <stdint.h>

#include "zconf.h"
#include "zutil.h"

/*
 * crc32_pclmul_simd_(): compute the IEEE crc32 of the buffer, where the
 * buffer length must be at least 64 and a multiple of 16.
 */
uint32_t ZLIB_INTERNAL crc32_pclmul_simd_(
    const unsigned char *buf,
    z_size_t len,
    uint32_t crc32);

/*
 * crc32_pclmul_simd_ buffer size constraint: see the use in zlib/crc32.c
 * for computing the crc32 of an arbitrary length buffer.
 */
#define Z_CRC32_SSE42_MINIMUM_LENGTH 64
#define Z_CRC32_SSE42_CHUNKSIZE_MASK 15
