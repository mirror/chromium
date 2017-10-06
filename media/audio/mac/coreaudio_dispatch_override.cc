// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/mac/coreaudio_dispatch_override.h"
#include "base/logging.h"

#include "base/atomicops.h"

#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

namespace media {
namespace {
const char kCoreAudioPath[] =
    "/System/Library/Frameworks/CoreAudio.framework/Versions/A/CoreAudio";

struct dyld_interpose_tuple {
  template <typename T>
  dyld_interpose_tuple(const T* replacement, const T* replacee)
      : replacement(reinterpret_cast<const void*>(replacement)),
        replacee(reinterpret_cast<const void*>(replacee)) {}
  const void* replacement;
  const void* replacee;
};

typedef void (*dyld_dynamic_interpose_f)(
    const struct mach_header* mh,
    const struct dyld_interpose_tuple array[],
    size_t count);

dispatch_queue_t g_pause_resume_queue = nullptr;
bool g_dispatch_override_installed = false;
dyld_dynamic_interpose_f g_dynamic_interpose_func = nullptr;
base::subtle::AtomicWord g_resumeio_callsite = 0;
base::subtle::AtomicWord g_pauseio_callsite = 0;

bool caller_is_pause_or_resume(intptr_t caller) {
  intptr_t resumeio_callsite =
      base::subtle::NoBarrier_Load(&g_resumeio_callsite);

  if (caller == resumeio_callsite)
    return true;

  intptr_t pauseio_callsite = base::subtle::NoBarrier_Load(&g_pauseio_callsite);
  if (caller == pauseio_callsite)
    return true;

  if (resumeio_callsite && pauseio_callsite)
    return false;

  // We don't know both callsites yet, so try to look up the caller.
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

dispatch_queue_t get_global_queue_override(long identifier,
                                           unsigned long flags) {
  // Get the return address
  const intptr_t* rbp = 0;
  asm("movq %%rbp, %0;" : "=r"(rbp));
  const intptr_t caller = rbp[1];

  // Check if it's one we should override
  if (identifier == DISPATCH_QUEUE_PRIORITY_HIGH &&
      caller_is_pause_or_resume(caller)) {
    return g_pause_resume_queue;
  }

  return dispatch_get_global_queue(identifier, flags);
}

void dispatch_override_add_image_func(const struct mach_header* mh,
                                      intptr_t vmaddr_slide) {
  if (!g_dispatch_override_installed) {
    Dl_info info = {};
    if (dladdr(mh, &info) == 0)
      return;

    if (strcmp(info.dli_fname, kCoreAudioPath) == 0) {
      dyld_interpose_tuple interposition(&get_global_queue_override,
                                         &dispatch_get_global_queue);
      g_dynamic_interpose_func(mh, &interposition, 1);
      g_dispatch_override_installed = true;
    }
  }
}

bool resolve_dynamic_interpose_func() {
  void* func = dlsym(RTLD_DEFAULT, "dyld_dynamic_interpose");
  if (func) {
    g_dynamic_interpose_func = reinterpret_cast<dyld_dynamic_interpose_f>(func);
    return true;
  }
  return false;
}

}  // namespace

bool initialize_coreaudio_dispatch_override() {
  DCHECK_EQ(g_dynamic_interpose_func, nullptr);
  DCHECK_EQ(g_pause_resume_queue, nullptr);

  if (resolve_dynamic_interpose_func()) {
    // TODO: Probably log this as well.
    g_pause_resume_queue = dispatch_queue_create("PauseResumeQueue", nullptr);
    _dyld_register_func_for_add_image(&dispatch_override_add_image_func);
    return true;
  }

  // TODO: Log this error, somehow.
  LOG(WARNING) << "Did not find dyld_dynamic_interpose(), hotfix disabled.";
  return false;
}

}  // namespace media
