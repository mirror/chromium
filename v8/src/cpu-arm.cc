// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

// CPU specific code for arm independent of OS goes here.

#include <sys/syscall.h>  // for cache flushing.

#include "v8.h"

#include "cpu.h"

namespace v8 { namespace internal {

void CPU::Setup() {
  // Nothing to do.
}


void CPU::FlushICache(void* start, size_t size) {
#if !defined (__arm__)
  // Not generating ARM instructions for C-code. This means that we are
  // building an ARM emulator based target. No I$ flushes are necessary.
#else
  // Ideally, we would call
  //   syscall(__ARM_NR_cacheflush, start,
  //           reinterpret_cast<intptr_t>(start) + size, 0);
  // however, syscall(int, ...) is not supported on all platforms, especially
  // not when using EABI, so we call the __ARM_NR_cacheflush syscall directly.

  register uint32_t beg asm("a1") = reinterpret_cast<uint32_t>(start);
  register uint32_t end asm("a2") =
      reinterpret_cast<uint32_t>(start) + size;
  register uint32_t flg asm("a3") = 0;
  #ifdef __ARM_EABI__
    register uint32_t scno asm("r7") = __ARM_NR_cacheflush;
    #if defined (__arm__) && !defined(__thumb__)
      // __arm__ may be defined in thumb mode.
      asm volatile(
          "swi 0x0"
          : "=r" (beg)
          : "0" (beg), "r" (end), "r" (flg), "r" (scno));
    #else
      asm volatile(
      "@   Enter ARM Mode  \n\t"
          "adr r3, 1f      \n\t"
          "bx  r3          \n\t"
          ".ALIGN 4        \n\t"
          ".ARM            \n"
      "1:  swi 0x0         \n\t"
      "@   Enter THUMB Mode\n\t"
          "adr r3, 2f+1    \n\t"
          "bx  r3          \n\t"
          ".THUMB          \n"
      "2:                  \n\t"
          : "=r" (beg)
          : "0" (beg), "r" (end), "r" (flg), "r" (scno)
          : "r3");
    #endif
  #else
    #if defined (__arm__) && !defined(__thumb__)
      // __arm__ may be defined in thumb mode.
      asm volatile(
          "swi %1"
          : "=r" (beg)
          : "i" (__ARM_NR_cacheflush), "0" (beg), "r" (end), "r" (flg));
    #else
      // Do not use the value of __ARM_NR_cacheflush in the inline assembly
      // below, because the thumb mode value would be used, which would be
      // wrong, since we switch to ARM mode before executing the swi instruction
      asm volatile(
      "@   Enter ARM Mode  \n\t"
          "adr r3, 1f      \n\t"
          "bx  r3          \n\t"
          ".ALIGN 4        \n\t"
          ".ARM            \n"
      "1:  swi 0x9f0002    \n"
      "@   Enter THUMB Mode\n\t"
          "adr r3, 2f+1    \n\t"
          "bx  r3          \n\t"
          ".THUMB          \n"
      "2:                  \n\t"
          : "=r" (beg)
          : "0" (beg), "r" (end), "r" (flg)
          : "r3");
    #endif
  #endif
#endif
}


void CPU::DebugBreak() {
#if !defined (__arm__)
  UNIMPLEMENTED();  // when building ARM emulator target
#else
  asm volatile("bkpt 0");
#endif
}

} }  // namespace v8::internal
