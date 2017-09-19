// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/thread_restrictions.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/test/gtest_util.h"
#include "base/threading/simple_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

class RunClosureThread : public SimpleThread {
 public:
  RunClosureThread(OnceClosure closure)
      : SimpleThread("ThreadRestrictionsTest"), closure_(std::move(closure)) {}

 private:
  void Run() override { std::move(closure_).Run(); }

  OnceClosure closure_;

  DISALLOW_COPY_AND_ASSIGN(RunClosureThread);
};

}  // namespace

TEST(ThreadRestrictionsTest, BlockingAllowedByDefault) {
  AssertBlockingAllowed();
}

TEST(ThreadRestrictionsTest, ScopedDisallowBlocking) {
  {
    ScopedDisallowBlocking scoped_disallow_blocking;
    EXPECT_DCHECK_DEATH({ AssertBlockingAllowed(); });
  }
  AssertBlockingAllowed();
}

TEST(ThreadRestrictionsTest, ScopedAllowBlocking) {
  ScopedDisallowBlocking scoped_disallow_blocking;
  {
    ScopedAllowBlocking scoped_allow_blocking;
    AssertBlockingAllowed();
  }
  EXPECT_DCHECK_DEATH({ AssertBlockingAllowed(); });
}

TEST(ThreadRestrictionsTest, ScopedAllowBlockingForTesting) {
  ScopedDisallowBlocking scoped_disallow_blocking;
  {
    ScopedAllowBlockingForTesting scoped_allow_blocking_for_testing;
    AssertBlockingAllowed();
  }
  EXPECT_DCHECK_DEATH({ AssertBlockingAllowed(); });
}

TEST(ThreadRestrictionsTest, BaseSyncPrimitivesAllowedByDefault) {
  AssertBaseSyncPrimitivesAllowed();
}

TEST(ThreadRestrictionsTest, DisallowBaseSyncPrimitives) {
  // Call DisallowBaseSyncPrimitives() on a separate thread because it
  // permanently disallows //base sync primitives on the calling thread. Wrap
  // the whole test within EXPECT_DCHECK_DEATH because using EXPECT_DCHECK_DEATH
  // in a process with multiple threads is discouraged.
  EXPECT_DCHECK_DEATH({
    RunClosureThread thread(BindOnce([]() {
      DisallowBaseSyncPrimitives();
      AssertBaseSyncPrimitivesAllowed();
    }));
    thread.Start();
    thread.Join();
  });
}

TEST(ThreadRestrictionsTest, ScopedAllowBaseSyncPrimitives) {
  // Call DisallowBaseSyncPrimitives() on a separate thread because it
  // permanently disallows //base sync primitives on the calling thread.
  RunClosureThread thread(BindOnce([]() {
    DisallowBaseSyncPrimitives();
    ScopedAllowBaseSyncPrimitives scoped_allow_base_sync_primitives;
    AssertBaseSyncPrimitivesAllowed();
  }));
  thread.Start();
  thread.Join();
}

TEST(ThreadRestrictionsTest, ScopedAllowBaseSyncPrimitivesResetsState) {
  // Call DisallowBaseSyncPrimitives() on a separate thread because it
  // permanently disallows //base sync primitives on the calling thread. Wrap
  // the whole test within EXPECT_DCHECK_DEATH because using EXPECT_DCHECK_DEATH
  // in a process with multiple threads is discouraged.
  EXPECT_DCHECK_DEATH({
    RunClosureThread thread(BindOnce([]() {
      DisallowBaseSyncPrimitives();
      { ScopedAllowBaseSyncPrimitives scoped_allow_base_sync_primitives; }
      AssertBaseSyncPrimitivesAllowed();
    }));
    thread.Start();
    thread.Join();
  });
}

TEST(ThreadRestrictionsTest,
     ScopedAllowBaseSyncPrimitivesWithBlockingDisallowed) {
  // Call DisallowBaseSyncPrimitives() on a separate thread because it
  // permanently disallows //base sync primitives on the calling thread. Wrap
  // the whole test within EXPECT_DCHECK_DEATH because using EXPECT_DCHECK_DEATH
  // in a process with multiple threads is discouraged.
  EXPECT_DCHECK_DEATH({
    RunClosureThread thread(BindOnce([]() {
      ScopedDisallowBlocking scoped_disallow_blocking;
      DisallowBaseSyncPrimitives();

      // This should DCHECK because blocking is not allowed in this scope
      // and OutsideBlockingScope is not passed to the constructor.
      ScopedAllowBaseSyncPrimitives scoped_allow_base_sync_primitives;
    }));
    thread.Start();
    thread.Join();
  });
}

TEST(ThreadRestrictionsTest,
     ScopedAllowBaseSyncPrimitivesOutsideBlockingScope) {
  RunClosureThread thread(BindOnce([]() {
    ScopedDisallowBlocking scoped_disallow_blocking;
    DisallowBaseSyncPrimitives();
    ScopedAllowBaseSyncPrimitives scoped_allow_base_sync_primitives{
        OutsideBlockingScope()};
    AssertBaseSyncPrimitivesAllowed();
  }));
  thread.Start();
  thread.Join();
}

TEST(ThreadRestrictionsTest, ScopedAllowBaseSyncPrimitivesForTesting) {
  // Call DisallowBaseSyncPrimitives() on a separate thread because it
  // permanently disallows //base sync primitives on the calling thread.
  RunClosureThread thread(BindOnce([]() {
    DisallowBaseSyncPrimitives();
    ScopedAllowBaseSyncPrimitivesForTesting
        scoped_allow_base_sync_primitives_for_testing;
    AssertBaseSyncPrimitivesAllowed();
  }));
  thread.Start();
  thread.Join();
}

TEST(ThreadRestrictionsTest,
     ScopedAllowBaseSyncPrimitivesForTestingResetsState) {
  // Call DisallowBaseSyncPrimitives() on a separate thread because it
  // permanently disallows //base sync primitives on the calling thread. Wrap
  // the whole test within EXPECT_DCHECK_DEATH because using EXPECT_DCHECK_DEATH
  // in a process with multiple threads is discouraged.
  EXPECT_DCHECK_DEATH({
    RunClosureThread thread(BindOnce([]() {
      DisallowBaseSyncPrimitives();
      {
        ScopedAllowBaseSyncPrimitivesForTesting
            scoped_allow_base_sync_primitives_for_testing;
      }
      AssertBaseSyncPrimitivesAllowed();
    }));
    thread.Start();
    thread.Join();
  });
}

TEST(ThreadRestrictionsTest,
     ScopedAllowBaseSyncPrimitivesForTestingWithBlockingDisallowed) {
  // Call DisallowBaseSyncPrimitives() on a separate thread because it
  // permanently disallows //base sync primitives on the calling thread.
  RunClosureThread thread(BindOnce([]() {
    ScopedDisallowBlocking scoped_disallow_blocking;
    DisallowBaseSyncPrimitives();
    // This should not DCHECK.
    ScopedAllowBaseSyncPrimitivesForTesting
        scoped_allow_base_sync_primitives_for_testing;
  }));
  thread.Start();
  thread.Join();
}

}  // namespace base
