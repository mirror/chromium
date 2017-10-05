// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_mac_dispatch_overrider.h"

#include "base/atomicops.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <string.h>

namespace {
const char kCoreAudioPath[] =
    "/System/Library/Frameworks/CoreAudio.framework/Versions/A/CoreAudio";

dispatch_queue_t g_chrome_dispatch_overrider_queue = nullptr;

base::subtle::AtomicWord g_address_range_base = 0;
base::subtle::AtomicWord g_address_range_top = 0;
base::subtle::AtomicWord g_resumeio_callsite = 0;
base::subtle::AtomicWord g_pauseio_callsite = 0;

// Despite atomicity, this may be called only once.
void chrome_dispatch_overrider_set_address_range(intptr_t base, intptr_t size) {
  base::subtle::Acquire_Store(&g_address_range_base, base);
  base::subtle::Release_Store(&g_address_range_top, base + size);
}

bool chrome_dispatch_overrider_has_address_range() {
  return base::subtle::NoBarrier_Load(&g_address_range_top) != 0;
}

void chrome_dispatch_overrider_add_image_func(const struct mach_header* mh,
                                              intptr_t vmaddr_slide) {
  if (!chrome_dispatch_overrider_has_address_range()) {
    Dl_info info = {};
    if (dladdr(mh, &info) == 0)
      return;

    if (strcmp(info.dli_fname, kCoreAudioPath) == 0) {
      // For now, check within a meg of the start of the library.
      chrome_dispatch_overrider_set_address_range(
          reinterpret_cast<intptr_t>(info.dli_fbase), 1024 * 1024);
    }
  }
}

bool chrome_dispatch_overrider_is_pause_or_resume(intptr_t caller) {
  intptr_t resumeio_callsite =
      base::subtle::NoBarrier_Load(&g_resumeio_callsite);
  intptr_t pauseio_callsite = base::subtle::NoBarrier_Load(&g_pauseio_callsite);

  if (caller == resumeio_callsite || caller == pauseio_callsite)
    return true;

  if (resumeio_callsite && pauseio_callsite)
    return false;

  // We don't know both callsites yet, so try to look up where we're coming
  // from.

  if (caller < base::subtle::NoBarrier_Load(&g_address_range_base) ||
      caller > base::subtle::NoBarrier_Load(&g_address_range_top))
    return false;

  Dl_info info;
  if (!dladdr(reinterpret_cast<const void*>(caller), &info))
    return false;

  if (strcmp(info.dli_fname, kCoreAudioPath) == 0) {
    if (!resumeio_callsite && info.dli_sname &&
        strcmp(info.dli_sname, "HALC_IOContext_ResumeIO") == 0) {
      resumeio_callsite = caller;
      base::subtle::NoBarrier_CompareAndSwap(&g_resumeio_callsite, 0,
                                             resumeio_callsite);
    }
    if (!pauseio_callsite && info.dli_sname &&
        strcmp(info.dli_sname, "HALC_IOContext_PauseIO") == 0) {
      pauseio_callsite = caller;
      base::subtle::NoBarrier_CompareAndSwap(&g_pauseio_callsite, 0,
                                             pauseio_callsite);
    }
  }

  return caller == pauseio_callsite || caller == resumeio_callsite;
}
}  // namespace

extern "C" {
__attribute__((used)) void chrome_dispatch_overrider_initialize() {
  assert(g_chrome_dispatch_overrider_queue == nullptr);
  g_chrome_dispatch_overrider_queue =
      dispatch_queue_create("PauseResumeQueue", nullptr);
  _dyld_register_func_for_add_image(&chrome_dispatch_overrider_add_image_func);
}

static dispatch_queue_t chrome_dispatch_overrider_get_global_queue(
    long identifier,
    unsigned long flags) {
  if (g_chrome_dispatch_overrider_queue &&
      chrome_dispatch_overrider_has_address_range()) {
    // Get the return address
    const intptr_t* rbp = 0;
    asm("movq %%rbp, %0;" : "=r"(rbp));
    const intptr_t caller = rbp[1];

    // Check if it's one we should override
    if (identifier == DISPATCH_QUEUE_PRIORITY_HIGH &&
        chrome_dispatch_overrider_is_pause_or_resume(caller)) {
      return g_chrome_dispatch_overrider_queue;
    }
  }

  return dispatch_get_global_queue(identifier, flags);
}

struct interposed_s {
  const void* new_func;
  const void* old_func;
};

__attribute__((used)) static struct interposed_s replacement
    __attribute__((section("__DATA,__interpose"))) = {
        (const void*)&chrome_dispatch_overrider_get_global_queue,
        (const void*)&dispatch_get_global_queue};
}
