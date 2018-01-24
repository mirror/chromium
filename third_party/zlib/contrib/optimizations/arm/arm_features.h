/* arm_features.h -- SoC features detection.
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#ifndef __ARM_FEATURES__
#define __ARM_FEATURES__
#include <stdbool.h>

extern bool arm_cpu_enable_crc32;
extern bool arm_cpu_enable_pmull;

void arm_check_features(void);

bool arm_supports_crc32();

bool arm_supports_pmull();

#endif
