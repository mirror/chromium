/* arm_features.c -- ARM processor features detection.
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Authors: Adenilson Cavalcanti <adenilson.cavalcanti@arm.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */
#include "arm_features.h"
#include "zutil.h"
#include <stdint.h>

#if defined(ARMV8_OS_ANDROID)
#include <cpu-features.h>
#elif defined(ARMV8_OS_LINUX)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#else
#error ### No ARM CPU features detection in your platform/OS
#endif

/* Keep the information concerning the features. */
bool ZLIB_INTERNAL arm_cpu_enable_crc32 = false;
bool ZLIB_INTERNAL arm_cpu_enable_pmull = false;

#include <pthread.h>

static pthread_once_t cpu_check_inited_once = PTHREAD_ONCE_INIT;
static void init_arm_features(void)
{
    uint32_t flag_crc32 = 0, flag_pmull = 0, capabilities = 0;

#if defined(ARMV8_OS_ANDROID)
    flag_crc32 = ANDROID_CPU_ARM_FEATURE_CRC32;
    flag_pmull = ANDROID_CPU_ARM_FEATURE_PMULL;
    capabilities = android_getCpuFeatures();
#elif defined(ARMV8_OS_LINUX)
    #if defined(__aarch64__)
        flag_crc32 = HWCAP_CRC32;
        flag_pmull = HWCAP_PMULL;
        capabilities = getauxval(AT_HWCAP);
    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
        /* The use of HWCAP2 is for getting features of newer ARMv8-A SoCs
         * while running in 32bits mode (i.e. aarch32).
         */
        flag_crc32 = HWCAP2_CRC32;
        flag_pmull = HWCAP2_PMULL;
        capabilities = getauxval(AT_HWCAP2);
    #endif
#endif

    if (capabilities & flag_crc32)
        arm_cpu_enable_crc32 = true;

    if (capabilities & flag_pmull)
        arm_cpu_enable_pmull = true;
}

void arm_check_features(void)
{
    pthread_once(&cpu_check_inited_once, init_arm_features);
}
