/* arm_features.h -- SoC features detection.
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#include "arm_features.h"
#include "zutil.h"
#include <stdint.h>
#if defined(ARMV8_OS_LINUX)
#include <sys/auxv.h>
#elif defined(ARMV8_OS_ANDROID)
#include <cpu-features.h>
#endif

/* Keep the information concerning the features. */
bool ZLIB_INTERNAL arm_cpu_enable_crc32 = false;
bool ZLIB_INTERNAL arm_cpu_enable_pmull = false;

/* TODO(cavalcantii): Should we handle Windows? Anyway this is gated in
 * the build.
 */
#include <pthread.h>
/* A read on manpage for pthread_once() suggests this to safely
 * dynamically perform initialization in C libraries.
 */
static pthread_once_t cpu_check_inited_once = PTHREAD_ONCE_INIT;
static void init_arm_features(void)
{
    uint32_t flag_crc32 = 0, flag_pmull = 0, capabilities = 0;
#if defined(ARMV8_OS_LINUX)
    /* TODO(cavalcantii): verify use of header file for this values
     * in the code base (i.e. skia, boringssl, etc).
     */
    flag_crc32 = (1 << 7);
    flag_pmull = (1 << 4);
    /* TODO(cavalcantii): when we enable CrOS, verify the scenario of
     * 32bits app running in a 64bits kernel (may require AT_HWCAP2).
     */
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

void arm_check_features(void)
{
    pthread_once(&cpu_check_inited_once, init_arm_features);
}
