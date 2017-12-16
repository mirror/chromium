/* arm_features.h -- SoC features detection.
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#ifndef __ARM_FEATURES__
#define __ARM_FEATURES__
#include <stdbool.h>

void arm_check_features(void);

bool arm_supports_crc32();

bool arm_supports_pmull();

#endif
