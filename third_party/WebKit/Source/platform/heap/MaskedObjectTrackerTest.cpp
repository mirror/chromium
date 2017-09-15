// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/heap/MaskedObjectTracker.h"

#include "platform/heap/Persistent.h"
#include "platform/heap/ThreadState.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

// Elements held by explicit GC type (not via a mixin).
// These do not require pointer adjustment to check liveness.
struct WithoutMixin {
  class ElementType : public GarbageCollected<ElementType> {
   public:
    DEFINE_INLINE_TRACE() {}
  };
  static ElementType* Create() { return new ElementType; }
};

// Elements held by mixin type.
// These do require pointer adjument to check liveness.
struct WithMixin {
  class ElementType : public GarbageCollectedMixin {};
  class ConcreteType : public GarbageCollected<ConcreteType>,
                       public ElementType {
    USING_GARBAGE_COLLECTED_MIXIN(ConcreteType);

   public:
    DEFINE_INLINE_TRACE() {}
  };
  static ElementType* Create() { return new ConcreteType; }
};

template <typename T>
class MaskedObjectTrackerTest : public ::testing::Test {};
using TestVariants = ::testing::Types<WithoutMixin, WithMixin>;
TYPED_TEST_CASE(MaskedObjectTrackerTest, TestVariants);

TYPED_TEST(MaskedObjectTrackerTest, InitialState) {
  MaskedObjectTracker<typename TypeParam::ElementType> tracker;
  EXPECT_FALSE(tracker.MatchesMask(~0u));
}

// The set of masks which match should be updated as elements are added.
TYPED_TEST(MaskedObjectTrackerTest, Add) {
  MaskedObjectTracker<typename TypeParam::ElementType> tracker;
  auto* a = TypeParam::Create();
  auto* b = TypeParam::Create();

  // Only masks which match 1 should work.
  tracker.Add(a, 1);
  EXPECT_TRUE(tracker.MatchesMask(1));
  EXPECT_FALSE(tracker.MatchesMask(2));
  EXPECT_TRUE(tracker.MatchesMask(3));

  // It is okay for multiple bits to be set.
  // Any query including the 2 bit or 4 bit works.
  tracker.Add(b, 6);
  EXPECT_TRUE(tracker.MatchesMask(1));
  EXPECT_TRUE(tracker.MatchesMask(2));
  EXPECT_TRUE(tracker.MatchesMask(3));

  // It is okay for the same object to be added with different masks.
  tracker.Add(a, 8);
  EXPECT_TRUE(tracker.MatchesMask(8));
}

// The set of masks which match should be updated as elements are removed.
TYPED_TEST(MaskedObjectTrackerTest, ExplicitRemoval) {
  MaskedObjectTracker<typename TypeParam::ElementType> tracker;
  auto* a = TypeParam::Create();
  auto* b = TypeParam::Create();

  tracker.Add(a, 1);
  tracker.Add(b, 2);
  tracker.Add(a, 6);
  EXPECT_TRUE(tracker.MatchesMask(1));
  EXPECT_TRUE(tracker.MatchesMask(2));
  EXPECT_TRUE(tracker.MatchesMask(3));
  EXPECT_TRUE(tracker.MatchesMask(4));

  tracker.Remove(a, 1);
  EXPECT_FALSE(tracker.MatchesMask(1));
  EXPECT_TRUE(tracker.MatchesMask(2));
  EXPECT_TRUE(tracker.MatchesMask(3));
  EXPECT_TRUE(tracker.MatchesMask(4));

  tracker.Remove(a, 6);
  EXPECT_FALSE(tracker.MatchesMask(1));
  EXPECT_TRUE(tracker.MatchesMask(2));
  EXPECT_TRUE(tracker.MatchesMask(3));
  EXPECT_FALSE(tracker.MatchesMask(4));

  tracker.Remove(b, 2);
  EXPECT_FALSE(tracker.MatchesMask(1));
  EXPECT_FALSE(tracker.MatchesMask(2));
  EXPECT_FALSE(tracker.MatchesMask(3));
  EXPECT_FALSE(tracker.MatchesMask(4));
}

// This is a hack for test purposes. The test below forces a GC to happen and
// claims that there are no GC pointers on the stack. For this to be valid, the
// tracker itself must live on the heap, not on the stack.
template <typename T>
struct MaskedObjectTrackerWrapper
    : public GarbageCollected<MaskedObjectTrackerWrapper<T>> {
  MaskedObjectTracker<T> tracker;

  DEFINE_INLINE_TRACE() { visitor->Trace(tracker); }
};

// The set of masks which match should be updated as elements are removed due to
// the garbage collector. Similar to the previous case, except all references to
// object |a| are removed together by the GC.
TYPED_TEST(MaskedObjectTrackerTest, ImplicitRemoval) {
  auto wrapper = WrapPersistent(
      new MaskedObjectTrackerWrapper<typename TypeParam::ElementType>);
  auto& tracker = wrapper->tracker;
  auto a = WrapPersistent(TypeParam::Create());
  auto b = WrapPersistent(TypeParam::Create());

  tracker.Add(a, 1);
  tracker.Add(b, 2);
  tracker.Add(a, 6);
  ThreadState::Current()->CollectAllGarbage();
  EXPECT_TRUE(tracker.MatchesMask(1));
  EXPECT_TRUE(tracker.MatchesMask(2));
  EXPECT_TRUE(tracker.MatchesMask(3));
  EXPECT_TRUE(tracker.MatchesMask(4));

  a.Clear();
  ThreadState::Current()->CollectAllGarbage();
  EXPECT_FALSE(tracker.MatchesMask(1));
  EXPECT_TRUE(tracker.MatchesMask(2));
  EXPECT_TRUE(tracker.MatchesMask(3));
  EXPECT_FALSE(tracker.MatchesMask(4));

  b.Clear();
  ThreadState::Current()->CollectAllGarbage();
  EXPECT_FALSE(tracker.MatchesMask(1));
  EXPECT_FALSE(tracker.MatchesMask(2));
  EXPECT_FALSE(tracker.MatchesMask(3));
  EXPECT_FALSE(tracker.MatchesMask(4));
}

}  // namespace
}  // namespace blink
