/* arm.h -- ARM CPU feature detection.
 *
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */

extern int arm_cpu_enable_crc32;
extern int arm_cpu_enable_pmull;

void arm_check_features(void);
