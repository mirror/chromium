// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/partition_allocator/spin_lock.h"

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
// you really should be using a proper lock (such as |base::Lock|)rather than
// these spinlocks.
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

namespace {

bool IsEvent(intptr_t current) {
  return current != base::subtle::SpinLock::UNLOCKED &&
         current != base::subtle::SpinLock::LOCKED &&
         current != base::subtle::SpinLock::CONTENDED;
}

}  // anonymous namespace

namespace base {
namespace subtle {

SpinLock::SpinLock() {
  lock_.store(UNLOCKED, std::memory_order_release);
}

SpinLock::~SpinLock() {
  intptr_t current = lock_.load(std::memory_order_acquire);
  if (UNLIKELY(IsEvent(current))) {
    WaitableEvent* event = reinterpret_cast<WaitableEvent*>(current);
    delete event;
  }
}

void SpinLock::LockSlow(intptr_t current) {
  // The value of |kYieldProcessorTries| is cargo culted from TCMalloc, Windows
  // critical section defaults, and various other recommendations. TODO(jschuh):
  // Further tuning may be warranted.
  static const int kYieldProcessorTries = 1000;

  // Spin until the lock becomes an event:
  do {
    for (int count = 0; count < kYieldProcessorTries; ++count) {
      // Let the processor know we're spinning.
      YIELD_PROCESSOR;
      current = lock_.load(std::memory_order_acquire);
      DCHECK_NE(UNLOCKED, current);
      if (LOCKED == current) {
        lock_.exchange(CONTENDED, std::memory_order_acquire);
      }
    }

    // Give the OS a chance to schedule something on this core.
    YIELD_THREAD;
  } while (!IsEvent(current));

  DCHECK(IsEvent(current));
  base::WaitableEvent* event = reinterpret_cast<base::WaitableEvent*>(current);
  event->Wait();
}

void SpinLock::UnlockSlow(intptr_t current) {
  WaitableEvent* event;
  if (IsEvent(current)) {
    event = reinterpret_cast<WaitableEvent*>(current);
  } else {
    event = new WaitableEvent(WaitableEvent::ResetPolicy::AUTOMATIC,
                              WaitableEvent::InitialState::NOT_SIGNALED);
    lock_.store(reinterpret_cast<intptr_t>(event), std::memory_order_release);
  }

  event->Signal();
}

}  // namespace subtle
}  // namespace base
