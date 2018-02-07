/* arm_features.h -- ARM processor features detection.
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */
#ifndef __ARM_FEATURES__
#define __ARM_FEATURES__
#include <stdbool.h>

/* Chromium zlib symbols mangling. */
#include "names.h"

extern bool arm_cpu_enable_crc32;
extern bool arm_cpu_enable_pmull;

void arm_check_features(void);

#endif
