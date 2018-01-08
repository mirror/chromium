/* arm_features.h -- SoC features detection.
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#include "arm_features.h"
#include <stdint.h>

#if defined(ARMV8_OS_LINUX)
#include <sys/auxv.h>
#elif defined(ARMV8_OS_ANDROID)
#include <cpu-features.h>
#endif

/* Keep the information concerning the features. */
#if defined(USE_ARMV8_CRC32)
#define ZLIB_INTERNAL __attribute__((visibility ("hidden")))
#else
#define ZLIB_INTERNAL
#endif
bool ZLIB_INTERNAL arm_cpu_enable_crc32 = false;
bool ZLIB_INTERNAL arm_cpu_enable_pmull = false;

/* TODO(cavalcantii): Should we handle Windows? Anyway this is gated in
 * the build.
 */
#ifdef USE_ARMV8_CRC32
#include <pthread.h>
/* A read on manpage for pthread_once() suggests this to safely
 * dynamically perform initialization in C libraries.
 */
static pthread_once_t cpu_check_inited_once = PTHREAD_ONCE_INIT;
static void init_arm_features(void)
{
    uint32_t flag_crc32 = 0, flag_pmull = 0, capabilities = 0;
#if defined(ARMV8_OS_LINUX)
    flag_crc32 = (1 << 7);
    flag_pmull = (1 << 4);
    capabilities = getauxval(AT_HWCAP);
#elif defined(ARMV8_OS_ANDROID)
    flag_crc32 = ANDROID_CPU_ARM_FEATURE_CRC32;
    flag_pmull = ANDROID_CPU_ARM_FEATURE_PMULL;
    capabilities = android_getCpuFeatures();
#endif
    if (capabilities & flag_crc32)
        arm_cpu_enable_crc32 = true;

    if (capabilities & flag_pmull)
        arm_cpu_enable_pmull = true;
}
#endif

void arm_check_features(void)
{
#ifdef USE_ARMV8_CRC32
    pthread_once(&cpu_check_inited_once, init_arm_features);
#endif
}

bool arm_supports_crc32()
{
    return arm_cpu_enable_crc32;
}

bool arm_supports_pmull()
{
    return arm_cpu_enable_pmull;
}
