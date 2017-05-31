// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_SYNCHRONIZATION_LOCK_H_
#define BASE_SYNCHRONIZATION_LOCK_H_

#include "base/base_export.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/synchronization/lock_impl.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"

namespace base {

// A convenient wrapper for an OS specific critical section.  The only real
// intelligence in this class is in debug mode for the support for the
// AssertAcquired() method.
class BASE_EXPORT Lock {
 public:
#if !DCHECK_IS_ON()
   // Optimized wrapper implementation
  Lock() : lock_() {}
  ~Lock() {}
  void Acquire() { lock_.Lock(); }
  void Release() { lock_.Unlock(); }

  // If the lock is not held, take it and return true. If the lock is already
  // held by another thread, immediately return false. This must not be called
  // by a thread already holding the lock (what happens is undefined and an
  // assertion may fail).
  bool Try() { return lock_.Try(); }

  // Null implementation if not debug.
  void AssertAcquired() const {}
#else
  Lock();
  ~Lock();

  // NOTE: We do not permit recursive locks and will commonly fire a DCHECK() if
  // a thread attempts to acquire the lock a second time (while already holding
  // it).
  void Acquire() {
    lock_.Lock();
    CheckUnheldAndMark();
  }
  void Release() {
    CheckHeldAndUnmark();
    lock_.Unlock();
  }

  bool Try() {
    bool rv = lock_.Try();
    if (rv) {
      CheckUnheldAndMark();
    }
    return rv;
  }

  void AssertAcquired() const;
#endif  // DCHECK_IS_ON()

#if defined(OS_POSIX) || defined(OS_WIN)
  // Both Windows and POSIX implementations of ConditionVariable need to be
  // able to see our lock and tweak our debugging counters, as they release and
  // acquire locks inside of their condition variable APIs.
  friend class ConditionVariable;
#endif

 private:
#if DCHECK_IS_ON()
  // Members and routines taking care of locks assertions.
  // Note that this checks for recursive locks and allows them
  // if the variable is set.  This is allowed by the underlying implementation
  // on windows but not on Posix, so we're doing unneeded checks on Posix.
  // It's worth it to share the code.
  void CheckHeldAndUnmark();
  void CheckUnheldAndMark();

  // All private data is implicitly protected by lock_.
  // Be VERY careful to only access members under that lock.
  base::PlatformThreadRef owning_thread_ref_;
#endif  // DCHECK_IS_ON()

  // Platform specific underlying lock implementation.
  internal::LockImpl lock_;

  DISALLOW_COPY_AND_ASSIGN(Lock);
};

// A helper class that acquires the given Lock while the AutoLock is in scope.
class AutoLock {
 public:
  struct AlreadyAcquired {};

  explicit AutoLock(Lock& lock) : lock_(lock) {
    lock_.Acquire();
  }

  AutoLock(Lock& lock, const AlreadyAcquired&) : lock_(lock) {
    lock_.AssertAcquired();
  }

  ~AutoLock() {
    lock_.AssertAcquired();
    lock_.Release();
  }

 private:
  Lock& lock_;
  DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

// AutoUnlock is a helper that will Release() the |lock| argument in the
// constructor, and re-Acquire() it in the destructor.
class AutoUnlock {
 public:
  explicit AutoUnlock(Lock& lock) : lock_(lock) {
    // We require our caller to have the lock.
    lock_.AssertAcquired();
    lock_.Release();
  }

  ~AutoUnlock() {
    lock_.Acquire();
  }

 private:
  Lock& lock_;
  DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
};

}  // namespace base

// Whether base::Lock mitigates priority inversion when used from different
// thread priorities.
//
// Windows mitigates priority inversion by randomly boosting the priority of
// ready threads.
// https://msdn.microsoft.com/library/windows/desktop/ms684831.aspx
//
// Mac mitigates priority inversion by setting the priority of a thread
// holding a Lock to the maximum priority of any other thread waiting on it.
//
// Other POSIX platforms could mitigate priority inversion the same way as Mac,
// but this feature is disabled because of security concerns.
//   * CVE-2010-0622 - wake_futex_pi unlocks incorrect, possible DoS.
//     https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2010-0622
//   * CVE-2012-6647 - Linux < 3.5.1, futex_wait_requeue_pi possible DoS.
//     https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2012-6647
//   * CVE-2014-3153 - Linux <= 3.14.5, futex_requeue, privilege escalation.
//     https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2014-3153
// Note that prior to glibc 2.17, priority inheritance mutexes may deadlock with
// condition variables during recacquisition of the mutex after the condition
// variable is signaled. This is not an issue for base/ since it isn't supposed
// to run with versions of glibc older than 2.17.
#if defined(OS_WIN) || defined(OS_MACOSX)
#define LOCK_HANDLES_MULTIPLE_THREAD_PRIORITES() 1
#elif defined(OS_POSIX)
#define LOCK_HANDLES_MULTIPLE_THREAD_PRIORITES() 0
#else
#error "Unsupported platform"
#endif

#endif  // BASE_SYNCHRONIZATION_LOCK_H_
