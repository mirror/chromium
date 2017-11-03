/* Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __ARMV8_CRC__
#define __ARMV8_CRC__

// Depending on the compiler flavor, size_t may be defined in
// one or the other header. See:
// http://stackoverflow.com/questions/26410466/gcc-linaro-compiler-throws-error-unknown-type-name-size-t
#include <stddef.h>
#include <stdint.h>

uint32_t armv8_crc32_little(uint32_t crc, const unsigned char* buf, size_t len);
#endif
