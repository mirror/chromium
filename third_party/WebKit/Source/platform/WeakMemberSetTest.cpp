// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/WeakMemberSet.h"
#include "platform/heap/Heap.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/heap/Member.h"
#include "platform/heap/ThreadState.h"
#include "platform/heap/TraceTraits.h"
#include "testing/gtest/include/gtest/gtest.h"

//#define TRACING

namespace blink {

void GC() {
  if (ThreadState::Current()->IsGCForbidden())
    return;
  ThreadState::Current()->CollectGarbage(BlinkGC::kNoHeapPointersOnStack,
                                         BlinkGC::kGCWithSweep,
                                         BlinkGC::kForcedGC);
}

class HeapInt : public GarbageCollected<HeapInt> {
 public:
  static HeapInt* Create(int x) { return new HeapInt(x); }

  DEFINE_INLINE_TRACE() {
#ifdef TRACING
    LOG(INFO) << "Traced " << this;
#endif
  }
  int Value() const { return x_; }

  bool operator==(const HeapInt& other) const {
    return other.Value() == Value();
  }

  unsigned GetHash() { return IntHash<int>::GetHash(x_); }

  HeapInt(int x) : x_(x) {}

 private:
  int x_;
};

class FinalizedHeapInt : public GarbageCollectedFinalized<FinalizedHeapInt> {
 public:
  EAGERLY_FINALIZE();
  static FinalizedHeapInt* Create(int x) { return new FinalizedHeapInt(x); }
  static int times_finalized_;
  static int times_traced_;

  DEFINE_INLINE_TRACE() {
#ifdef TRACING
    LOG(INFO) << "Traced " << this;
#endif
    times_traced_++;
  }
  int Value() const { return x_; }

  bool operator==(const FinalizedHeapInt& other) const {
    return other.Value() == Value();
  }

  unsigned GetHash() { return IntHash<int>::GetHash(x_); }

  FinalizedHeapInt(int x) : x_(x) {}
  ~FinalizedHeapInt() {
#ifdef TRACING
    LOG(INFO) << "Finalized " << this;
#endif
    times_finalized_++;
  }

 private:
  int x_;
};

int FinalizedHeapInt::times_finalized_ = 0;
int FinalizedHeapInt::times_traced_ = 0;

class TestWeakMemberSetContainer
    : public GarbageCollected<TestWeakMemberSetContainer> {
 public:
  static TestWeakMemberSetContainer* Create() {
    return new TestWeakMemberSetContainer();
  }

  DEFINE_INLINE_TRACE() {
    LOG(INFO) << "Traced TestWeakMemberSetContainer" << this;
    visitor->Trace(elem_);
  }

  void Add(int x) {
    HeapInt* h = HeapInt::Create(x);
    elem_->insert(h);
  }

  TestWeakMemberSetContainer() {}

 private:
  Member<WeakMemberSet<HeapInt>> elem_ = WeakMemberSet<HeapInt>::Create();
};

TEST(WeakMemberSetTest, HeapInt) {
  HeapInt* h = HeapInt::Create(4);
  EXPECT_EQ(h->Value(), 4);
}

TEST(WeakMemberSetTest, HeapIntPersistent) {
  Persistent<HeapInt> h = HeapInt::Create(4);
  EXPECT_EQ(h->Value(), 4);
}

TEST(WeakMemberSetTest, HeapIntPersistentWithGC) {
  Persistent<HeapInt> h = HeapInt::Create(4);
  auto ptr = h.Get();
  EXPECT_EQ(h->Value(), 4);
  GC();
  EXPECT_EQ(h.Get(), ptr);
  EXPECT_EQ(h->Value(), 4);
}

TEST(WeakMemberSetTest, HeapIntIsHeapAllocated) {
  ThreadHeap& heap = ThreadState::Current()->Heap();
  auto old = heap.ObjectPayloadSizeForTesting();
  HeapInt* h = HeapInt::Create(4);
  EXPECT_EQ(h->Value(), 4);
  EXPECT_GT(heap.ObjectPayloadSizeForTesting(), old);
}

TEST(WeakMemberSetTest, WeakMemberSetIsHeapAllocated) {
  ThreadHeap& heap = ThreadState::Current()->Heap();
  auto old = heap.ObjectPayloadSizeForTesting();
  HeapInt* h = HeapInt::Create(4);
  EXPECT_EQ(h->Value(), 4);
  EXPECT_GT(heap.ObjectPayloadSizeForTesting(), old);
}

TEST(WeakMemberSetTest, SizeShouldBeZeroOnConstruction) {
  WeakMemberSet<HeapInt> as;
  EXPECT_EQ(as.size(), 0u);
}

TEST(WeakMemberSetTest, InsertWeak) {
  WeakMemberSet<HeapInt> as;
  auto v1 = HeapInt::Create(4);
  as.insert(v1);
  EXPECT_EQ(as.size(), 1u);
}

TEST(WeakMemberSetTest, IterateWeak) {
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();
  for (int i = 0; i < 32; i++) {
    as->insert(HeapInt::Create(i));
  }
  auto cur = 0;
  for (auto t : *as) {
    EXPECT_EQ(t->Value(), cur);
    cur++;
  }
}

TEST(WeakMemberSetTest, IterateWeakAt) {
  constexpr int max = 32;
  WeakMemberSet<HeapInt> as;
  for (int i = 0; i < max; i++) {
    auto h = HeapInt::Create(i);
    as.insert(h);
  }
  for (int i = 0; i < max; i++) {
    EXPECT_EQ(as.at(i)->Value(), i);
  }
}

TEST(WeakMemberSetTest, TestContains) {
  Persistent<HeapInt> h = HeapInt::Create(0x500);
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();
  as->insert(h);
  EXPECT_TRUE(as->Contains(h));
}

TEST(WeakMemberSetTest, TestContainsValue) {
  Persistent<HeapInt> h = HeapInt::Create(0x500);
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();
  as->insert(h);
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(0x500)));
}

TEST(WeakMemberSetTest, TestContainsMultiple) {
  constexpr int padding = 5;

  // apVector<Member<HeapInt>, 2> hv;
  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }

  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
  }
}

//
// These tests check various compaction scenarios. Compaction occurs
// after a garbage collection pass has invalidated WeakMember elements
// of WeakMemberSet.
//

TEST(WeakMemberSetTest, SizeShouldBeZeroAfterGCRefsExpired) {
  // We can't always return a cached size because if size() is called
  // straight after a garbage collection, we don't know the true size.
  constexpr int max = 32;
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();
  for (int i = 0; i < max; i++) {
    auto h = HeapInt::Create(i);
    as->insert(h);
  }
  GC();
  ASSERT_EQ(as->size(), 0u);
}

TEST(WeakMemberSetTest, ShouldBeConsistentAfterOneGCRefExpiresAtStart) {
  // This test checks whether the compaction method can handle one
  // element getting garbage collected at the start of the backing vector.
  constexpr int padding = 8;
  constexpr int special = 0x500;

  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  // Create a special thing that will be garbage collected
  // at the start of the WeakMemberSet.
  {
    auto s = HeapInt::Create(special);
    as->insert(s);
  }
  // Stuff the back of the WeakMemberSet with elements, distinct from
  // the elements at the front.
  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }

  // Do a well-formedness check - the WeakMemberSet should contain
  // all the elements currently in the HeapVector, plus the special element.
  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
  }
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special)));

  // Trigger a garbage collection, which will eliminate the special element
  // and move up all subsequent elements in the list.
  GC();
  as->compact();

  // The WeakMemberSet should not contain the special element since it's been
  // garbage collected.
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special)));
  for (const auto& it : *hv) {
    // But, it should still contain all the HeapVector elements, since
    // the HeapVector stores Persistent references to them.
    EXPECT_TRUE(as->Contains(it));
  }
}

TEST(WeakMemberSetTest, ShouldBeConsistentAfterTwoGCRefExpireAtStart) {
  // This test tests whether we can lazily compact by setting the insert pointer
  // and whether the insert() method handles that correctly.
  constexpr int padding = 8;
  constexpr int special1 = 0x500;
  constexpr int special2 = 0x501;

  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  // Create two elements at the stat
  // Create a special thing that will be garbage collected
  // at the start of the WeakMemberSet.
  {
    auto s = HeapInt::Create(special1);
    as->insert(s);
  }
  {
    auto s = HeapInt::Create(special2);
    as->insert(s);
  }
  // Stuff the back of the WeakMemberSet with elements, distinct from
  // the elements at the front.
  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }

  // Do a well-formedness check - the WeakMemberSet should contain
  // all the elements currently in the HeapVector, plus the special element.
  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
  }
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special1)));
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special2)));

  // Trigger a garbage collection, which will eliminate the special elements
  // and move up all subsequent elements in the list.
  GC();
  as->compact();

  // The WeakMemberSet should not contain the special element since it's been
  // garbage collected.
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special1)));
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special2)));
  for (const auto& it : *hv) {
    // But, it should still contain all the HeapVector elements, since
    // the HeapVector stores Persistent references to them.
    EXPECT_TRUE(as->Contains(it));
  }
}

TEST(WeakMemberSetTest, ShouldBeConsistentAfterOneGCRefExpiresAtCenter) {
  // This test checks whether the compaction method can handle one
  // element expiring in the center of the backing vector.
  constexpr int padding = 8;
  constexpr int special = 0x500;

  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  // Stuff the front of the WeakMemberSet with elements
  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }
  // Create a special thing that will be garbage collected
  // in the center of the WeakMemberSet.
  {
    auto s = HeapInt::Create(special);
    as->insert(s);
  }
  // Stuff the back of the WeakMemberSet with elements, distinct from
  // the elements at the front.
  for (int i = padding; i < 2 * padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }

  // Do a well-formedness check - the WeakMemberSet should contain
  // all the elements currently in the HeapVector, plus the special element.
  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
    EXPECT_TRUE(as->ContainsValue(it));
  }
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special)));

  // Trigger a garbage collection, which will eliminate the special element
  // and move up all subsequent elements in the list.
  GC();
  as->compact();

  // The WeakMemberSet should not contain the special element since it's been
  // garbage collected.
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special)));
  for (const auto& it : *hv) {
    // But, it should still contain all the HeapVector elements, since
    // the HeapVector stores Persistent references to them.
    EXPECT_TRUE(as->Contains(it));
  }
}

TEST(WeakMemberSetTest, ShouldBeConsistentAfterOneGCRefExpiresAtEnd) {
  // This test checks whether the compaction method can handle one
  // element expiring at the end of the backing vector.
  constexpr int padding = 8;
  constexpr int special = 0x500;

  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  // Stuff the front of the WeakMemberSet with elements
  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }
  // Create a special thing that will be garbage collected
  // in the center of the WeakMemberSet.
  {
    auto s = HeapInt::Create(special);
    as->insert(s);
  }

  // Do a well-formedness check - the WeakMemberSet should contain
  // all the elements currently in the HeapVector, plus the special element.
  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
    EXPECT_TRUE(as->ContainsValue(it));
  }
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special)));

  // Trigger a garbage collection, which will eliminate the special element
  // and move up all subsequent elements in the list.
  GC();
  as->compact();

  // The WeakMemberSet should not contain the special element since it's been
  // garbage collected.
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special)));
  for (const auto& it : *hv) {
    // But, it should still contain all the HeapVector elements, since
    // the HeapVector stores Persistent references to them.
    EXPECT_TRUE(as->Contains(it));
    EXPECT_TRUE(as->ContainsValue(it));
  }
}

TEST(WeakMemberSetTest, TestInsertAfterGC) {
  // This test checks whether the WeakMemberSet can insert a new element into a
  // slot made available after GC
  constexpr int padding = 8;
  constexpr int special = 0x500;

  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  // Stuff the front of the WeakMemberSet with elements
  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }
  // Create a special element in the middle which will be garbage collected
  {
    Persistent<HeapInt> h = HeapInt::Create(padding);
    as->insert(h);
  }
  // Stuff the back of the WeakMemberSet with elements
  for (int i = padding + 1; i < 2 * padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }

  GC();

  Persistent<HeapInt> h = HeapInt::Create(special);
  as->insert(h);
  for (int i = 0; i < padding; i++) {
    EXPECT_TRUE(as->ContainsValue(HeapInt::Create(i)));
  }
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special)));
  for (int i = padding + 1; i < 2 * padding; i++) {
    EXPECT_TRUE(as->ContainsValue(HeapInt::Create(i)));
  }
}

TEST(WeakMemberSetTest, IterateWeakAtWithGC) {
  constexpr int max = 32;
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();
  for (int i = 0; i < max; i++) {
    auto h = HeapInt::Create(i);
    as->insert(h);
  }
  GC();
  for (size_t i = 0; i < as->size(); i++) {
    LOG(INFO) << as->at(i);
  }
}

TEST(WeakMemberSetTest, IterateWeakWithGC) {
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();
  for (int i = 0; i < 128; i++) {
    as->insert(HeapInt::Create(i));
  }
  GC();
  auto cur = 0;
  for (WeakMember<HeapInt> t : *as) {
    EXPECT_EQ(*t, cur);
    ASSERT_TRUE(ThreadHeap::IsHeapObjectAlive(t));
    cur++;
  }
}

TEST(WeakMemberSetTest, TestFinalization) {
  {
    Persistent<TestWeakMemberSetContainer> c =
        TestWeakMemberSetContainer::Create();
    c->Add(4);
    c->Add(5);
  }
  GC();
}

TEST(WeakMemberSetTest, SwapWorksEqual) {
  // This test checks whether we can swap two WeakMemberSets' contents when
  // they both have an equal number of elements.

  constexpr int padding = 8;

  Persistent<HeapVector<Member<HeapInt>>> hv1 =
      new HeapVector<Member<HeapInt>>();
  Persistent<HeapVector<Member<HeapInt>>> hv2 =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as1 = WeakMemberSet<HeapInt>::Create();
  Persistent<WeakMemberSet<HeapInt>> as2 = WeakMemberSet<HeapInt>::Create();

  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv1->push_back(h);
    as1->insert(h);
  }
  for (int i = padding; i < 2 * padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv2->push_back(h);
    as2->insert(h);
  }
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as1->Contains(it));
    EXPECT_TRUE(as1->ContainsValue(it));
    EXPECT_FALSE(as2->Contains(it));
    EXPECT_FALSE(as2->ContainsValue(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_TRUE(as2->Contains(it));
    EXPECT_TRUE(as2->ContainsValue(it));
    EXPECT_FALSE(as1->Contains(it));
    EXPECT_FALSE(as1->ContainsValue(it));
  }

  as1->swap(*as2);
  for (const auto& it : *hv1) {
    EXPECT_FALSE(as1->Contains(it));
    EXPECT_FALSE(as1->ContainsValue(it));
    EXPECT_TRUE(as2->Contains(it));
    EXPECT_TRUE(as2->ContainsValue(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_FALSE(as2->Contains(it));
    EXPECT_FALSE(as2->ContainsValue(it));
    EXPECT_TRUE(as1->Contains(it));
    EXPECT_TRUE(as1->ContainsValue(it));
  }
}

TEST(WeakMemberSetTest, SwapWorksRhsLarger) {
  // This test checks whether we can swap two WeakMemberSets' contents when
  // the thing we're swapping with has a larger number of elements.

  constexpr int padding = 8;
  constexpr int extra = 4;

  Persistent<HeapVector<Member<HeapInt>>> hv1 =
      new HeapVector<Member<HeapInt>>();
  Persistent<HeapVector<Member<HeapInt>>> hv2 =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as1 = WeakMemberSet<HeapInt>::Create();
  Persistent<WeakMemberSet<HeapInt>> as2 = WeakMemberSet<HeapInt>::Create();

  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv1->push_back(h);
    as1->insert(h);
  }
  for (int i = padding; i < 2 * padding + extra; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv2->push_back(h);
    as2->insert(h);
  }
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as1->Contains(it));
    EXPECT_FALSE(as2->Contains(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_TRUE(as2->Contains(it));
    EXPECT_FALSE(as1->Contains(it));
  }

  as1->swap(*as2);
  for (const auto& it : *hv1) {
    EXPECT_FALSE(as1->Contains(it));
    EXPECT_TRUE(as2->Contains(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_FALSE(as2->Contains(it));
    EXPECT_TRUE(as1->Contains(it));
  }
}

TEST(WeakMemberSetTest, SwapWorksRhsSmaller) {
  // This test checks whether we can swap two WeakMemberSets' contents when
  // the thing we're swapping with has a smaller number of elements.

  constexpr int padding = 8;
  constexpr int extra = 4;

  Persistent<HeapVector<Member<HeapInt>>> hv1 =
      new HeapVector<Member<HeapInt>>();
  Persistent<HeapVector<Member<HeapInt>>> hv2 =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as1 = WeakMemberSet<HeapInt>::Create();
  Persistent<WeakMemberSet<HeapInt>> as2 = WeakMemberSet<HeapInt>::Create();

  for (int i = 0; i < padding + extra; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv1->push_back(h);
    as1->insert(h);
  }
  for (int i = padding + extra; i < 2 * padding + extra; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv2->push_back(h);
    as2->insert(h);
  }
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as1->Contains(it));
    EXPECT_FALSE(as2->Contains(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_TRUE(as2->Contains(it));
    EXPECT_FALSE(as1->Contains(it));
  }

  as1->swap(*as2);

  for (const auto& it : *hv1) {
    EXPECT_FALSE(as1->Contains(it));
    EXPECT_TRUE(as2->Contains(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_FALSE(as2->Contains(it));
    EXPECT_TRUE(as1->Contains(it));
  }
}

TEST(WeakMemberSetTest, SwapWorksRhsEmpty) {
  // This test checks whether we can swap with an empty WeakMemberSet,
  // Particularly important in the context of LifeCycleObserver.

  constexpr unsigned int padding = 8;

  Persistent<HeapVector<Member<HeapInt>>> hv1 =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as1 = WeakMemberSet<HeapInt>::Create();
  Persistent<WeakMemberSet<HeapInt>> as2 = WeakMemberSet<HeapInt>::Create();

  // Fill the reference set with elements
  for (unsigned int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv1->push_back(h);
    as1->insert(h);
  }
  // Check that everything's well-formed
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as1->Contains(it));
    EXPECT_FALSE(as2->Contains(it));
  }
  EXPECT_EQ(as1->size(), padding);
  EXPECT_EQ(as2->size(), 0u);

  // Swap with the empty set
  as1->swap(*as2);

  EXPECT_EQ(as1->size(), 0u);
  EXPECT_EQ(as2->size(), padding);

  // Check that all the elements in as1 made it to as2
  for (const auto& it : *hv1) {
    EXPECT_FALSE(as1->Contains(it));
    EXPECT_TRUE(as2->Contains(it));
  }

  // Check that there are no remaining elements in as1
  for (const auto& it : *as1) {
    EXPECT_EQ(it, nullptr);
  }
}

TEST(WeakMemberSetTest, SwapWorksRhsEmptyWithoutPersistent) {
  // This test checks whether we can swap with an empty WeakMemberSet,
  // Particularly important in the context of LifeCycleObserver.

  constexpr unsigned int padding = 8;

  Persistent<WeakMemberSet<HeapInt>> as1 = WeakMemberSet<HeapInt>::Create();
  Persistent<WeakMemberSet<HeapInt>> as2 = WeakMemberSet<HeapInt>::Create();

  // Fill the reference set with elements
  for (unsigned int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    as1->insert(h);
  }
  EXPECT_EQ(as1->size(), padding);
  EXPECT_EQ(as2->size(), 0u);

  // Swap with the empty set
  as1->swap(*as2);

  EXPECT_EQ(as1->size(), 0u);
  EXPECT_EQ(as2->size(), padding);
}

TEST(WeakMemberSetTest, SwapWorksAfterGC) {
  // This test checks whether the system transfers the need for compaction
  // onto another
  // This test checks whether the compaction method can handle one
  // element expiring at the end of the backing vector.
  constexpr int padding = 8;
  constexpr int special = 0x500;

  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();
  Persistent<WeakMemberSet<HeapInt>> empty = WeakMemberSet<HeapInt>::Create();
  // Create a special thing that will be garbage collected
  // in the center of the WeakMemberSet.
  {
    auto s = HeapInt::Create(special);
    as->insert(s);
  }
  // Stuff the front of the WeakMemberSet with elements
  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }
  // Do a well-formedness check - the WeakMemberSet should contain
  // all the elements currently in the HeapVector, plus the special element.
  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
    EXPECT_TRUE(as->ContainsValue(it));
  }
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special)));

  // Trigger a garbage collection, which will eliminate the special element
  // and require compaction.
  GC();

  // Swap with an empty list, which trivially does not require compaction.
  as->swap(*empty);

  // Everything should be in the same order at this stage
  auto it1 = hv->begin();
  auto it2 = empty->begin();

  while (it1 != hv->end() && it2 != empty->end()) {
//  EXPECT_EQ(*it1, *it2);
#ifdef TRACING
    LOG(INFO) << it1->Get()->Value();
    LOG(INFO) << it2.Get()->Value();
#endif
    EXPECT_EQ(it1->Get()->Value(), it2.Get()->Value());
    ++it1;
    ++it2;
  }
}

TEST(WeakMemberSetTest, ShouldBeConsistentAfterMultipleGCRefsExpire) {
  // This test checks whether the compaction method can handle
  // multiple elements expiring throughout the backing vector,
  // including two or more elements in succession.

  constexpr int padding = 8;
  constexpr int special = 0x500;

  Persistent<HeapVector<Member<HeapInt>>> hv =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  {
    // Garbage collect this element at the start
    auto s = HeapInt::Create(special);
    as->insert(s);
  }

  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);  // 9
  }
  {
    // Garbage collect this element at the the first 1/3rd
    auto s = HeapInt::Create(special);
    as->insert(s);  // 10
  }

  for (int i = padding; i < 2 * padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);  // 18
  }
  {
    // Garbage collect this element at the the 2nd 1/3rd
    auto s = HeapInt::Create(special * 2);
    as->insert(s);  // 19
  }
  {
    // Garbage collect this element at the the 2nd 1/3rd
    auto s = HeapInt::Create(special * 2 + 1);
    as->insert(s);  // 20
  }
  for (int i = 2 * padding; i < 3 * padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);  // 28
  }
  {
    // Garbage collect this element at the the 2nd 1/3rd
    auto s = HeapInt::Create(special * 3);
    as->insert(s);  // 29
  }

  // Do a well-formedness check - the WeakMemberSet should contain
  // all the elements currently in the HeapVector, plus the special elements
  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
    EXPECT_TRUE(as->ContainsValue(it));
  }
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special)));
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special * 2)));
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special * 2 + 1)));
  EXPECT_TRUE(as->ContainsValue(HeapInt::Create(special * 3)));
  EXPECT_EQ(as->size(), padding * 3u + 5u);

  GC();
  unsigned int cur = 0;
  for (const auto& it : *hv) {
    EXPECT_TRUE(as->Contains(it));
    EXPECT_TRUE(as->ContainsValue(it));
    cur++;
  }
  EXPECT_EQ(cur, padding * 3u);
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special)));
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special * 2)));
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special * 2 + 1)));
  EXPECT_FALSE(as->ContainsValue(HeapInt::Create(special * 3)));
  EXPECT_EQ(as->size(), padding * 3u);
}

TEST(WeakMemberSetTest, TestEraseElementWhenExists) {
  // This test checks whether it's possible to erase elements which we know
  // exist
  constexpr int padding = 8;
  Persistent<HeapVector<Member<HeapInt>>> hv1 =
      new HeapVector<Member<HeapInt>>();
  Persistent<HeapVector<Member<HeapInt>>> hv2 =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    if (i % 2 == 0) {
      hv1->push_back(h);
    } else {
      hv2->push_back(h);
    }
    as->insert(h);
  }
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as->Contains(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_TRUE(as->Contains(it));
  }

#ifdef TRACING
  for (const auto& it : *as) {
    LOG(INFO) << it->Value();
  }
#endif
  // Remove the elements from hv2
  for (const auto& it : *hv2) {
    as->erase(it);
  }
  as->compact();
#ifdef TRACING
  for (const auto& it : *as) {
    LOG(INFO) << it->Value();
  }
#endif
  for (const auto& it : *hv2) {
    EXPECT_FALSE(as->Contains(it));
  }
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as->Contains(it));
  }
}

TEST(WeakMemberSetTest, TestEraseElementWhenNotExists) {
  // This test checks whether removing non-existent elements crashes the program
  constexpr int padding = 8;
  Persistent<HeapVector<Member<HeapInt>>> hv1 =
      new HeapVector<Member<HeapInt>>();
  Persistent<HeapVector<Member<HeapInt>>> hv2 =
      new HeapVector<Member<HeapInt>>();
  Persistent<WeakMemberSet<HeapInt>> as = WeakMemberSet<HeapInt>::Create();

  for (int i = 0; i < padding; i++) {
    Persistent<HeapInt> h = HeapInt::Create(i);
    if (i % 2 == 0) {
      hv1->push_back(h);
      as->insert(h);
    } else {
      hv2->push_back(h);
    }
  }
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as->Contains(it));
  }
  for (const auto& it : *hv2) {
    EXPECT_FALSE(as->Contains(it));
  }

  // Remove the elements from hv2 (which don't exist)
  for (const auto& it : *hv2) {
    as->erase(it);
  }
  for (const auto& it : *hv2) {
    EXPECT_FALSE(as->Contains(it));
  }
  for (const auto& it : *hv1) {
    EXPECT_TRUE(as->Contains(it));
  }
}

TEST(WeakMemberSetTest, FinalizedHeapIntTraced) {
  int cur = FinalizedHeapInt::times_traced_;
  Persistent<FinalizedHeapInt> f = FinalizedHeapInt::Create(4);
  GC();
  ASSERT_GT(FinalizedHeapInt::times_traced_, cur);
}

TEST(WeakMemberSetTest, FinalizedHeapIntFinalized) {
  int cur = FinalizedHeapInt::times_finalized_;
  {
    Persistent<FinalizedHeapInt> f = FinalizedHeapInt::Create(4);
#ifdef TRACING
    LOG(INFO) << cur << " " << FinalizedHeapInt::times_finalized_ << " "
              << f->Value();
#endif
  }
  GC();
#ifdef TRACING
  LOG(INFO) << cur << " " << FinalizedHeapInt::times_finalized_;
#endif
  ASSERT_GT(FinalizedHeapInt::times_finalized_, cur);
}

TEST(WeakMemberSetTest, FinalizedHeapIntTracedOnlyOnceInWeakMemberSet) {
  int cur = FinalizedHeapInt::times_traced_;
  Persistent<WeakMemberSet<FinalizedHeapInt>> as =
      WeakMemberSet<FinalizedHeapInt>::Create();
  Persistent<FinalizedHeapInt> h = FinalizedHeapInt::Create(4);
  as->insert(h);
  GC();

  ASSERT_EQ(FinalizedHeapInt::times_traced_, cur + 1);
}

TEST(WeakMemberSetTest, FinalizedHeapIntTracedInWeakMemberSet) {
  // This test checks that Trace events propagate correctly.
  constexpr int padding = 8;
  Persistent<HeapVector<Member<FinalizedHeapInt>>> hv =
      new HeapVector<Member<FinalizedHeapInt>>();
  Persistent<WeakMemberSet<FinalizedHeapInt>> as =
      WeakMemberSet<FinalizedHeapInt>::Create();

  int cur = FinalizedHeapInt::times_traced_;

  for (int i = 0; i < padding; i++) {
    Persistent<FinalizedHeapInt> h = FinalizedHeapInt::Create(i);
    hv->push_back(h);
    as->insert(h);
  }

  GC();

  ASSERT_EQ(FinalizedHeapInt::times_traced_, cur + padding);
}

TEST(WeakMemberSetTest, FinalizedHeapIntFinalizedInWeakMemberSet) {
  // This test checks that Trace events propagate correctly.
  constexpr int padding = 8;
  Persistent<WeakMemberSet<FinalizedHeapInt>> as =
      WeakMemberSet<FinalizedHeapInt>::Create();

  int cur = FinalizedHeapInt::times_finalized_;

  for (int i = 0; i < padding; i++) {
    Persistent<FinalizedHeapInt> h = FinalizedHeapInt::Create(i);
    as->insert(h);
    GC();
  }

  GC();
  ASSERT_EQ(as->size(), 0u);

  // GE rather than EQ here because sometimes finalizers sometimes run
  // on objects created during tests in this period.
  ASSERT_GE(FinalizedHeapInt::times_finalized_, cur + padding);
}

}  // namespace blink
