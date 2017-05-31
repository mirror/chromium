// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/lock_impl.h"

#include <errno.h>
#include <string.h>

#include "base/debug/activity_tracker.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"

namespace base {
namespace internal {

LockImpl::LockImpl() {
  pthread_mutexattr_t mta;
  int rv = pthread_mutexattr_init(&mta);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
#if LOCK_HANDLES_MULTIPLE_THREAD_PRIORITES()
  rv = pthread_mutexattr_setprotocol(&mta, PTHREAD_PRIO_INHERIT);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
#endif
#ifndef NDEBUG
  // In debug, setup attributes for lock error checking.
  rv = pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_ERRORCHECK);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
#endif
  rv = pthread_mutex_init(&native_handle_, &mta);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
  rv = pthread_mutexattr_destroy(&mta);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
}

LockImpl::~LockImpl() {
  int rv = pthread_mutex_destroy(&native_handle_);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
}

bool LockImpl::Try() {
  int rv = pthread_mutex_trylock(&native_handle_);
  DCHECK(rv == 0 || rv == EBUSY) << ". " << strerror(rv);
  return rv == 0;
}

void LockImpl::Lock() {
  base::debug::ScopedLockAcquireActivity lock_activity(this);
  int rv = pthread_mutex_lock(&native_handle_);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
}

void LockImpl::Unlock() {
  int rv = pthread_mutex_unlock(&native_handle_);
  DCHECK_EQ(rv, 0) << ". " << strerror(rv);
}

}  // namespace internal
}  // namespace base
