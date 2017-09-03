// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ALLOCATOR_PARTITION_ALLOCATOR_LOCK_UTIL_H
#define BASE_ALLOCATOR_PARTITION_ALLOCATOR_LOCK_UTIL_H

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <sched.h>
#endif

// The YIELD_PROCESSOR macro wraps an architecture specific-instruction that
// informs the processor we're in a busy wait, so it can handle the branch more
// intelligently and e.g. reduce power to our core or give more resources to the
// other hyper-thread on this core. See the following for context:
// https://software.intel.com/en-us/articles/benefitting-power-and-performance-sleep-loops
//
// The YIELD_THREAD macro tells the OS to relinquish our quantum. This is
// basically a worst-case fallback, and if you're hitting it with any frequency
// you really should be using a proper lock (such as |base::Lock|).

#if defined(OS_WIN)
#define YIELD_PROCESSOR YieldProcessor()
#define YIELD_THREAD SwitchToThread()
#elif defined(COMPILER_GCC) || defined(__clang__)
#if defined(ARCH_CPU_X86_64) || defined(ARCH_CPU_X86)
#define YIELD_PROCESSOR __asm__ __volatile__("pause")
#elif defined(ARCH_CPU_ARMEL) || defined(ARCH_CPU_ARM64)
#define YIELD_PROCESSOR __asm__ __volatile__("yield")
#elif defined(ARCH_CPU_MIPSEL)
// The MIPS32 docs state that the PAUSE instruction is a no-op on older
// architectures (first added in MIPS32r2). To avoid assembler errors when
// targeting pre-r2, we must encode the instruction manually.
#define YIELD_PROCESSOR __asm__ __volatile__(".word 0x00000140")
#elif defined(ARCH_CPU_MIPS64EL) && __mips_isa_rev >= 2
// Don't bother doing using .word here since r2 is the lowest supported mips64
// that Chromium supports.
#define YIELD_PROCESSOR __asm__ __volatile__("pause")
#endif
#endif

#ifndef YIELD_PROCESSOR
#warning "Processor yield not supported on this architecture."
#define YIELD_PROCESSOR ((void)0)
#endif

#ifndef YIELD_THREAD
#if defined(OS_POSIX)
#define YIELD_THREAD sched_yield()
#else
#warning "Thread yield not supported on this OS."
#define YIELD_THREAD ((void)0)
#endif
#endif

#endif  // BASE_ALLOCATOR_PARTITION_ALLOCATOR_LOCK_UTIL_H
