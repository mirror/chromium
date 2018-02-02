/* armv8_crc32.h -- CRC32 checksums using ARMv8-a crypto instructions.
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */
#ifndef __ARMV8_CRC__
#define __ARMV8_CRC__

/* Depending on the compiler flavor, size_t may be defined in
 * one or the other header. See:
 * http://stackoverflow.com/questions/26410466
 */
#include <stddef.h>
#include <stdint.h>

unsigned long armv8_crc32_little(unsigned long crc,
                                 const unsigned char* buf,
                                 size_t len);
#endif
