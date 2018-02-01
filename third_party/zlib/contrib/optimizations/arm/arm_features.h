/* arm_features.h -- SoC features detection.
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */
#ifndef __ARM_FEATURES__
#define __ARM_FEATURES__
#include <stdbool.h>

/* A bit nasty as this are 'public' but we save a few cycles
 * by accessing it directly in crc32() instead of using functions
 * to return the values.
 */
extern bool arm_cpu_enable_crc32;
extern bool arm_cpu_enable_pmull;

void arm_check_features(void);

#endif
