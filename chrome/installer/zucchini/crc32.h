// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_CRC_32_H_
#define CHROME_INSTALLER_ZUCCHINI_CRC_32_H_

#include <stddef.h>
#include <stdint.h>

namespace zucchini {

// Calculates Crc of the given buffer.
uint32_t crc32(const uint8_t* first, const uint8_t* last);

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_CRC_32_H_
