// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/heap/CallbackStack.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/heap/Heap.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/heap/Member.h"
#include "platform/heap/ThreadState.h"
#include "platform/heap/TraceTraits.h"
#include "platform/heap/Visitor.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace incremental_marking_test {

class IncrementalMarkingScope {
 public:
  explicit IncrementalMarkingScope(ThreadState* thread_state)
      : gc_forbidden_scope_(thread_state),
        thread_state_(thread_state),
        heap_(thread_state_->Heap()),
        marking_stack_(heap_.MarkingStack()) {
    EXPECT_TRUE(marking_stack_->IsEmpty());
    marking_stack_->Commit();
    heap_.EnableIncrementalMarkingBarrier();
    thread_state->current_gc_data_.visitor =
        Visitor::Create(thread_state, Visitor::kGlobalMarking);
  }

  ~IncrementalMarkingScope() {
    EXPECT_TRUE(marking_stack_->IsEmpty());
    heap_.DisableIncrementalMarkingBarrier();
    marking_stack_->Decommit();
  }

  CallbackStack* marking_stack() const { return marking_stack_; }

 protected:
  ThreadState::GCForbiddenScope gc_forbidden_scope_;
  ThreadState* const thread_state_;
  ThreadHeap& heap_;
  CallbackStack* const marking_stack_;
};

template <typename T>
class ExpectWriteBarrierFires : public IncrementalMarkingScope {
 public:
  ExpectWriteBarrierFires(ThreadState* thread_state, T* object)
      : IncrementalMarkingScope(thread_state),
        object_(object),
        header_(HeapObjectHeader::FromPayload(object_)) {
    EXPECT_FALSE(header_->IsMarked());
  }

  ~ExpectWriteBarrierFires() {
    EXPECT_FALSE(marking_stack_->IsEmpty());
    EXPECT_TRUE(header_->IsMarked());
    CallbackStack::Item* item = marking_stack_->Pop();
    EXPECT_EQ(object_, item->Object());
  }

 private:
  T* const object_;
  HeapObjectHeader* const header_;
};

template <typename T>
class ExpectNoWriteBarrierFires : public IncrementalMarkingScope {
 public:
  ExpectNoWriteBarrierFires(ThreadState* thread_state, T* object)
      : IncrementalMarkingScope(thread_state),
        object_(object),
        header_(HeapObjectHeader::FromPayload(object_)) {
    EXPECT_TRUE(marking_stack_->IsEmpty());
    EXPECT_TRUE(header_->IsMarked());
  }

  ~ExpectNoWriteBarrierFires() {
    EXPECT_TRUE(marking_stack_->IsEmpty());
    EXPECT_TRUE(header_->IsMarked());
  }

 private:
  T* const object_;
  HeapObjectHeader* const header_;
};

class Dummy : public GarbageCollected<Dummy> {
 public:
  static Dummy* Create() { return new Dummy(); }

  void set_next(Dummy* next) { next_ = next; }

  virtual void Trace(blink::Visitor* visitor) { visitor->Trace(next_); }

 private:
  Dummy() : next_(nullptr) {}

  Member<Dummy> next_;
};

TEST(IncrementalMarkingTest, EnableDisableBarrier) {
  Dummy* dummy = Dummy::Create();
  BasePage* page = PageFromObject(dummy);
  ThreadHeap& heap = ThreadState::Current()->Heap();
  EXPECT_FALSE(page->IsIncrementalMarking());
  EXPECT_FALSE(ThreadState::Current()->IsIncrementalMarking());
  heap.EnableIncrementalMarkingBarrier();
  EXPECT_TRUE(page->IsIncrementalMarking());
  EXPECT_TRUE(ThreadState::Current()->IsIncrementalMarking());
  heap.DisableIncrementalMarkingBarrier();
  EXPECT_FALSE(page->IsIncrementalMarking());
  EXPECT_FALSE(ThreadState::Current()->IsIncrementalMarking());
}

TEST(IncrementalMarkingTest, WriteBarrierOnUnmarkedObject) {
  Dummy* parent = Dummy::Create();
  Dummy* child = Dummy::Create();
  HeapObjectHeader* child_header = HeapObjectHeader::FromPayload(child);
  {
    ExpectWriteBarrierFires<Dummy> scope(ThreadState::Current(), child);
    EXPECT_FALSE(child_header->IsMarked());
    parent->set_next(child);
    EXPECT_TRUE(child_header->IsMarked());
  }
}

TEST(IncrementalMarkingTest, NoWriteBarrierOnMarkedObject) {
  Dummy* parent = Dummy::Create();
  Dummy* child = Dummy::Create();
  HeapObjectHeader* child_header = HeapObjectHeader::FromPayload(child);
  child_header->Mark();
  {
    ExpectNoWriteBarrierFires<Dummy> scope(ThreadState::Current(), child);
    parent->set_next(child);
  }
}

TEST(IncrementalMarkingTest, ManualWriteBarrierTriggersWhenMarkingIsOn) {
  Dummy* dummy = Dummy::Create();
  HeapObjectHeader* dummy_header = HeapObjectHeader::FromPayload(dummy);
  {
    ExpectWriteBarrierFires<Dummy> scope(ThreadState::Current(), dummy);
    EXPECT_FALSE(dummy_header->IsMarked());
    ThreadState::Current()->Heap().WriteBarrier(dummy);
    EXPECT_TRUE(dummy_header->IsMarked());
  }
}

TEST(IncrementalMarkingTest, ManualWriteBarrierBailoutWhenMarkingIsOff) {
  Dummy* dummy = Dummy::Create();
  HeapObjectHeader* dummy_header = HeapObjectHeader::FromPayload(dummy);

  ThreadHeap& heap = ThreadState::Current()->Heap();
  CallbackStack* marking_stack = heap.MarkingStack();
  EXPECT_TRUE(marking_stack->IsEmpty());
  EXPECT_FALSE(dummy_header->IsMarked());
  heap.WriteBarrier(dummy);
  EXPECT_FALSE(dummy_header->IsMarked());
  EXPECT_TRUE(marking_stack->IsEmpty());
}

namespace {

class Mixin : public GarbageCollectedMixin {
 public:
  Mixin() : next_(nullptr) {}
  virtual ~Mixin() {}

  virtual void Trace(blink::Visitor* visitor) { visitor->Trace(next_); }

  virtual void Bar() {}

 protected:
  Member<Dummy> next_;
};

class ClassWithVirtual {
 protected:
  virtual void Foo() {}
};

class Child : public GarbageCollected<Child>,
              public ClassWithVirtual,
              public Mixin {
  USING_GARBAGE_COLLECTED_MIXIN(Child);

 public:
  static Child* Create() { return new Child(); }
  virtual ~Child() {}

  virtual void Trace(blink::Visitor* visitor) { Mixin::Trace(visitor); }

  virtual void Foo() {}
  virtual void Bar() {}

 protected:
  Child() : ClassWithVirtual(), Mixin() {}
};

class ParentWithMixinPointer : public GarbageCollected<ParentWithMixinPointer> {
 public:
  static ParentWithMixinPointer* Create() {
    return new ParentWithMixinPointer();
  }

  void set_mixin(Mixin* mixin) { mixin_ = mixin; }

  virtual void Trace(blink::Visitor* visitor) { visitor->Trace(mixin_); }

 protected:
  ParentWithMixinPointer() : mixin_(nullptr) {}

  Member<Mixin> mixin_;
};

}  // namespace

TEST(IncrementalMarkingTest, WriteBarrierOnUnmarkedMixinApplication) {
  ParentWithMixinPointer* parent = ParentWithMixinPointer::Create();
  Child* child = Child::Create();
  HeapObjectHeader* child_header = HeapObjectHeader::FromPayload(child);
  Mixin* mixin = static_cast<Mixin*>(child);
  EXPECT_NE(static_cast<void*>(child), static_cast<void*>(mixin));
  {
    ExpectWriteBarrierFires<Child> scope(ThreadState::Current(), child);
    EXPECT_FALSE(child_header->IsMarked());
    parent->set_mixin(mixin);
    EXPECT_TRUE(child_header->IsMarked());
  }
}

TEST(IncrementalMarkingTest, NoWriteBarrierOnMarkedMixinApplication) {
  ParentWithMixinPointer* parent = ParentWithMixinPointer::Create();
  Child* child = Child::Create();
  HeapObjectHeader* child_header = HeapObjectHeader::FromPayload(child);
  child_header->Mark();
  Mixin* mixin = static_cast<Mixin*>(child);
  EXPECT_NE(static_cast<void*>(child), static_cast<void*>(mixin));
  {
    ExpectNoWriteBarrierFires<Child> scope(ThreadState::Current(), child);
    EXPECT_TRUE(child_header->IsMarked());
    parent->set_mixin(mixin);
    EXPECT_TRUE(child_header->IsMarked());
  }
}

TEST(IncrementalMarkingTest, HeapVectorAssumptions) {
  static_assert(std::is_trivially_move_assignable<Member<Dummy>>::value,
                "Member<T> should not be trivially move assignable");
  static_assert(std::is_trivially_copy_assignable<Member<Dummy>>::value,
                "Member<T> should not be trivially copy assignable");
}

TEST(IncrementalMarkingTest, HeapVectorOfMemberPushBack) {
  Dummy* dummy = Dummy::Create();
  HeapVector<Member<Dummy>> vec;
  {
    ExpectWriteBarrierFires<Dummy> scope(ThreadState::Current(), dummy);
    vec.push_back(dummy);
  }
}

TEST(IncrementalMarkingTest, HeapVectorOfMemberEmplaceBack) {
  Dummy* dummy = Dummy::Create();
  HeapVector<Member<Dummy>> vec;
  {
    ExpectWriteBarrierFires<Dummy> scope(ThreadState::Current(), dummy);
    vec.emplace_back(dummy);
  }
}

TEST(IncrementalMarkingTest, HeapVectorOfPairOfMemberPushBack) {
  Dummy* dummy1 = Dummy::Create();
  Dummy* dummy2 = Dummy::Create();
  HeapObjectHeader* dummy1_header = HeapObjectHeader::FromPayload(dummy1);
  HeapObjectHeader* dummy2_header = HeapObjectHeader::FromPayload(dummy2);
  HeapVector<std::pair<Member<Dummy>, Member<Dummy>>> vec;
  {
    IncrementalMarkingScope scope(ThreadState::Current());
    EXPECT_FALSE(dummy1_header->IsMarked());
    EXPECT_FALSE(dummy2_header->IsMarked());
    vec.push_back(std::make_pair(Member<Dummy>(dummy1), Member<Dummy>(dummy2)));
    EXPECT_TRUE(dummy1_header->IsMarked());
    EXPECT_TRUE(dummy2_header->IsMarked());
    EXPECT_FALSE(scope.marking_stack()->IsEmpty());
    CallbackStack::Item* item = scope.marking_stack()->Pop();
    EXPECT_EQ(dummy2, item->Object());
    EXPECT_FALSE(scope.marking_stack()->IsEmpty());
    item = scope.marking_stack()->Pop();
    EXPECT_EQ(dummy1, item->Object());
  }
}

TEST(IncrementalMarkingTest, HeapVectorOfPairOfMemberEmplaceBack) {
  Dummy* dummy1 = Dummy::Create();
  Dummy* dummy2 = Dummy::Create();
  HeapObjectHeader* dummy1_header = HeapObjectHeader::FromPayload(dummy1);
  HeapObjectHeader* dummy2_header = HeapObjectHeader::FromPayload(dummy2);
  HeapVector<std::pair<Member<Dummy>, Member<Dummy>>> vec;
  {
    IncrementalMarkingScope scope(ThreadState::Current());
    EXPECT_FALSE(dummy1_header->IsMarked());
    EXPECT_FALSE(dummy2_header->IsMarked());
    vec.emplace_back(dummy1, dummy2);
    EXPECT_TRUE(dummy1_header->IsMarked());
    EXPECT_TRUE(dummy2_header->IsMarked());
    EXPECT_FALSE(scope.marking_stack()->IsEmpty());
    CallbackStack::Item* item = scope.marking_stack()->Pop();
    EXPECT_EQ(dummy2, item->Object());
    EXPECT_FALSE(scope.marking_stack()->IsEmpty());
    item = scope.marking_stack()->Pop();
    EXPECT_EQ(dummy1, item->Object());
  }
}

namespace {

class Dummy2 {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  Dummy2(Dummy* x, int y) : x_(x), y_(y) {}

  virtual ~Dummy2() {}
  virtual void Trace(blink::Visitor* visitor) { visitor->Trace(x_); }

 private:
  Member<Dummy> x_;
  int y_;
};

}  // namespace

TEST(IncrementalMarkingTest, HeapVectorField) {
  Dummy* dummy = Dummy::Create();
  HeapVector<Dummy2> vec;
  {
    ExpectWriteBarrierFires<Dummy> scope(ThreadState::Current(), dummy);
    vec.emplace_back(dummy, 1);
  }
}

}  // namespace incremental_marking_test
}  // namespace blink
