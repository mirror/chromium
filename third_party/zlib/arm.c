/* arm.c -- ARM CPU feature detection.
 *
 * Copyright 2018 The Chromium Authors. All rights reserved.
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
/* FIXME: fuchsia zircon OS ignored: it has no {auxv,hwcap}.h support */
#else
#error CPU feature detection is not defined for your ARM device
#endif

#include <endian.h>
#include <pthread.h>

static pthread_once_t cpu_check_inited_once = PTHREAD_ONCE_INIT;

static void _arm_check_features(void);

void arm_check_features(void)
{
    pthread_once(&cpu_check_inited_once, _arm_check_features);
}

static void _arm_check_features(void)
{
    /* http://bit.ly/2CcoEsr for run-time detection of ARMv8 features */
#if __BYTE_ORDER == __LITTLE_ENDIAN
    /* zlib uses these features on little endian ARMv8 devices */
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
#endif
#endif /* LITTLE_ENDIAN */
}
