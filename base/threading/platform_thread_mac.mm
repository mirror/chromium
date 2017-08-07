// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/platform_thread.h"

#import <Foundation/Foundation.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <stddef.h>
#include <sys/resource.h>

#include <algorithm>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mach_logging.h"
#include "base/threading/thread_id_name_manager.h"
#include "base/tracked_objects.h"
#include "build/build_config.h"

namespace base {

namespace {
NSString* const kThreadPriorityKey = @"CrThreadPriorityKey";
}  // namespace

// If Cocoa is to be used on more than one thread, it must know that the
// application is multithreaded.  Since it's possible to enter Cocoa code
// from threads created by pthread_thread_create, Cocoa won't necessarily
// be aware that the application is multithreaded.  Spawning an NSThread is
// enough to get Cocoa to set up for multithreaded operation, so this is done
// if necessary before pthread_thread_create spawns any threads.
//
// http://developer.apple.com/documentation/Cocoa/Conceptual/Multithreading/CreatingThreads/chapter_4_section_4.html
void InitThreading() {
  static BOOL multithreaded = [NSThread isMultiThreaded];
  if (!multithreaded) {
    // +[NSObject class] is idempotent.
    [NSThread detachNewThreadSelector:@selector(class)
                             toTarget:[NSObject class]
                           withObject:nil];
    multithreaded = YES;

    DCHECK([NSThread isMultiThreaded]);
  }
}

// static
void PlatformThread::SetName(const std::string& name) {
  //ThreadIdNameManager::GetInstance()->SetName(CurrentId(), name);
  //tracked_objects::ThreadData::InitializeThreadContext(name);

  // Mac OS X does not expose the length limit of the name, so
  // hardcode it.
  const int kMaxNameLength = 63;
  std::string shortened_name = name.substr(0, kMaxNameLength);
  // pthread_setname() fails (harmlessly) in the sandbox, ignore when it does.
  // See http://crbug.com/47058
  pthread_setname_np(shortened_name.c_str());
}

namespace {

// Enables time-contraint policy and priority suitable for low-latency,
// glitch-resistant audio.
void SetPriorityRealtimeAudio() {
}

}  // anonymous namespace

// static
bool PlatformThread::CanIncreaseCurrentThreadPriority() {
  return true;
}

// static
void PlatformThread::SetCurrentThreadPriority(ThreadPriority priority) {
  // Changing the priority of the main thread causes performance regressions.
  // https://crbug.com/601270
  DCHECK(![[NSThread currentThread] isMainThread]);

  switch (priority) {
    case ThreadPriority::BACKGROUND:
      [[NSThread currentThread] setThreadPriority:0];
      break;
    case ThreadPriority::NORMAL:
    case ThreadPriority::DISPLAY:
      [[NSThread currentThread] setThreadPriority:0.5];
      break;
    case ThreadPriority::REALTIME_AUDIO:
      SetPriorityRealtimeAudio();
      DCHECK_EQ([[NSThread currentThread] threadPriority], 1.0);
      break;
  }

  [[[NSThread currentThread] threadDictionary]
      setObject:@(static_cast<int>(priority))
         forKey:kThreadPriorityKey];
}

// static
ThreadPriority PlatformThread::GetCurrentThreadPriority() {
  NSNumber* priority = base::mac::ObjCCast<NSNumber>([[[NSThread currentThread]
      threadDictionary] objectForKey:kThreadPriorityKey]);

  if (!priority)
    return ThreadPriority::NORMAL;

  ThreadPriority thread_priority =
      static_cast<ThreadPriority>(priority.intValue);
  switch (thread_priority) {
    case ThreadPriority::BACKGROUND:
    case ThreadPriority::NORMAL:
    case ThreadPriority::DISPLAY:
    case ThreadPriority::REALTIME_AUDIO:
      return thread_priority;
    default:
      NOTREACHED() << "Unknown priority.";
      return ThreadPriority::NORMAL;
  }
}

size_t GetDefaultThreadStackSize(const pthread_attr_t& attributes) {
#if defined(OS_IOS)
  return 0;
#else
  // The Mac OS X default for a pthread stack size is 512kB.
  // Libc-594.1.4/pthreads/pthread.c's pthread_attr_init uses
  // DEFAULT_STACK_SIZE for this purpose.
  //
  // 512kB isn't quite generous enough for some deeply recursive threads that
  // otherwise request the default stack size by specifying 0. Here, adopt
  // glibc's behavior as on Linux, which is to use the current stack size
  // limit (ulimit -s) as the default stack size. See
  // glibc-2.11.1/nptl/nptl-init.c's __pthread_initialize_minimal_internal. To
  // avoid setting the limit below the Mac OS X default or the minimum usable
  // stack size, these values are also considered. If any of these values
  // can't be determined, or if stack size is unlimited (ulimit -s unlimited),
  // stack_size is left at 0 to get the system default.
  //
  // Mac OS X normally only applies ulimit -s to the main thread stack. On
  // contemporary OS X and Linux systems alike, this value is generally 8MB
  // or in that neighborhood.
  size_t default_stack_size = 0;
  struct rlimit stack_rlimit;
  if (pthread_attr_getstacksize(&attributes, &default_stack_size) == 0 &&
      getrlimit(RLIMIT_STACK, &stack_rlimit) == 0 &&
      stack_rlimit.rlim_cur != RLIM_INFINITY) {
    default_stack_size =
        std::max(std::max(default_stack_size,
                          static_cast<size_t>(PTHREAD_STACK_MIN)),
                 static_cast<size_t>(stack_rlimit.rlim_cur));
  }
  return default_stack_size;
#endif
}

void TerminateOnThread() {
}

}  // namespace base
