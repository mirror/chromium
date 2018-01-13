/* arm.c -- ARM CPU feature detection.
 *
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */

#include "arm.h"

#include "zutil.h"

int ZLIB_INTERNAL arm_cpu_enable_crc32 = 0;
int ZLIB_INTERNAL arm_cpu_enable_pmull = 0;

#if defined(ARMV8_OS_ANDROID)
#include <cpu-features.h>
#elif defined(ARMV8_OS_LINUX)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#elif defined(ARMV8_OS_ZIRCON)
#include <zircon/arch/arm64/feature.h>
#if defined(__Fuchsia__)
#pragma message __Fuchsia__ is defined
#endif
#if defined(__fuchsia__)
#pragma message __fuchsia__ is defined
#endif
#if defined(__aarch64__)
#pragma message __aarch64__ is defined
#endif
#error arm.c hello there fuchsia_arm64
#else
#error arm.c CPU feature detection is not defined for your build target
#endif

#include <pthread.h>

static pthread_once_t cpu_check_inited_once = PTHREAD_ONCE_INIT;

static void _arm_check_features(void);

void arm_check_features(void)
{
    pthread_once(&cpu_check_inited_once, _arm_check_features);
}

static void _arm_check_features(void)
{
   /*
    * Run-time detection of ARMv8 CPU features: see http://bit.ly/2CcoEsr
    */
#if defined(ARMV8_OS_ANDROID) && defined(__aarch64__)
    uint64_t cpu_feature = android_getCpuFeatures();
    arm_cpu_enable_crc32 = !!(cpu_feature & ANDROID_CPU_ARM64_FEATURE_CRC32);
    arm_cpu_enable_pmull = !!(cpu_feature & ANDROID_CPU_ARM64_FEATURE_PMULL);
#elif defined(ARMV8_OS_ANDROID) /* aarch32 */
    uint64_t cpu_feature = android_getCpuFeatures();
    arm_cpu_enable_crc32 = !!(cpu_feature & ANDROID_CPU_ARM_FEATURE_CRC32);
    arm_cpu_enable_pmull = !!(cpu_feature & ANDROID_CPU_ARM_FEATURE_PMULL);
#elif defined(ARMV8_OS_LINUX) && defined(__aarch64__)
    unsigned long cpu_feature = getauxval(AT_HWCAP);
    arm_cpu_enable_crc32 = !!(cpu_feature & HWCAP_CRC32);
    arm_cpu_enable_pmull = !!(cpu_feature & HWCAP_PMULL);
#elif defined(ARMV8_OS_LINUX) /* aarch32 */
    unsigned long cpu_feature = getauxval(AT_HWCAP2);
    arm_cpu_enable_crc32 = !!(cpu_feature & HWCAP2_CRC32);
    arm_cpu_enable_pmull = !!(cpu_feature & HWCAP2_PMULL);
#elif defined(ARMV8_OS_ZIRCON) /* aarch64 */
#endif
}
