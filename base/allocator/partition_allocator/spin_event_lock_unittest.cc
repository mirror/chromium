// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include "base/allocator/partition_allocator/event_lock.h"
#include "base/allocator/partition_allocator/spin_lock.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/threading/thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

static const size_t kBufferSize = 16;

static subtle::SpinLock g_spin_lock;
static subtle::EventLock g_event_lock;

static void FillBuffer(volatile char* buffer, char fill_pattern) {
  for (size_t i = 0; i < kBufferSize; ++i)
    buffer[i] = fill_pattern;
}

static void ChangeAndCheckBuffer(volatile char* buffer) {
  FillBuffer(buffer, '\0');
  int total = 0;
  for (size_t i = 0; i < kBufferSize; ++i)
    total += buffer[i];

  EXPECT_EQ(0, total);

  // This will mess with the other thread's calculation if we accidentally get
  // concurrency.
  FillBuffer(buffer, '!');
}

template <typename LockType>
static void ThreadMain(volatile char* buffer, LockType* lock) {
  for (int i = 0; i < 500000; ++i) {
    typename LockType::Guard guard(*lock);
    ChangeAndCheckBuffer(buffer);
  }
}

template <typename LockType>
static void TortureLock(LockType* lock) {
  char shared_buffer[kBufferSize];

  Thread thread1("thread1");
  Thread thread2("thread2");

  thread1.StartAndWaitForTesting();
  thread2.StartAndWaitForTesting();

  thread1.task_runner()->PostTask(
      FROM_HERE, BindOnce(&ThreadMain<LockType>,
                          Unretained(static_cast<char*>(shared_buffer)), lock));
  thread2.task_runner()->PostTask(
      FROM_HERE, BindOnce(&ThreadMain<LockType>,
                          Unretained(static_cast<char*>(shared_buffer)), lock));
}

TEST(SpinLockTest, Torture) {
  TortureLock<subtle::SpinLock>(&g_spin_lock);
}

TEST(EventLockTest, Torture) {
  TortureLock<subtle::EventLock>(&g_event_lock);
}

}  // namespace base
