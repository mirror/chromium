// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CompositorMutatorImpl.h"
#include "platform/graphics/CompositorMutatorClient.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class CompositorMutatorImplTest : public ::testing::Test {
 public:
 private:
  void SetUp() override {}
};

#define EXPECT_NOT_NULL(p) EXPECT_NE((void*)0, p)

TEST_F(CompositorMutatorImplTest, Creation) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::Create();

  EXPECT_NOT_NULL(mutator);
  EXPECT_FALSE(mutator->HasAnimators());

  mutator = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator);

  CompositorMutatorImpl* mutator2 = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator2);
  EXPECT_NE(mutator, mutator2);
  EXPECT_FALSE(mutator->HasAnimators());
  EXPECT_FALSE(mutator2->HasAnimators());
}

class EmptyCompositorAnimator final
    : public GarbageCollected<EmptyCompositorAnimator>,
      public CompositorAnimator {
  USING_GARBAGE_COLLECTED_MIXIN(EmptyCompositorAnimator);

 public:
  void Mutate(double m) {
    now_ = m;
    count_++;
  }

  // What has been done to you?
  double TestLast() const { return now_; };
  int TestCount() const { return count_; };

  EmptyCompositorAnimator() : now_(0), count_(0) {}

 private:
  double now_;
  int count_;
};

TEST_F(CompositorMutatorImplTest, AnimatorRegistration) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::CreateForTest();
  EXPECT_NOT_NULL(mutator);
  EXPECT_FALSE(mutator->HasAnimators());

  EmptyCompositorAnimator* anim1 = new EmptyCompositorAnimator;

  mutator->RegisterCompositorAnimator(anim1);
  EXPECT_TRUE(mutator->HasAnimators());

  mutator->UnregisterCompositorAnimator(anim1);
  EXPECT_FALSE(mutator->HasAnimators());
}

TEST_F(CompositorMutatorImplTest, AnimatorMutation) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::CreateForTest();
  EXPECT_NOT_NULL(mutator);
  EXPECT_FALSE(mutator->HasAnimators());

  EmptyCompositorAnimator* anim1 = new EmptyCompositorAnimator();
  EmptyCompositorAnimator* anim2 = new EmptyCompositorAnimator();
  EXPECT_EQ(0, anim1->TestLast());
  EXPECT_EQ(0, anim1->TestCount());
  EXPECT_EQ(0, anim2->TestLast());
  EXPECT_EQ(0, anim2->TestCount());

  // Mutation with no animators - OK, and effect-free.
  const double testvalue1 = 3.1416;
  mutator->Mutate(testvalue1);
  EXPECT_EQ(0, anim1->TestLast());
  EXPECT_EQ(0, anim1->TestCount());

  // Mutation with a single animator registered.
  mutator->RegisterCompositorAnimator(anim1);

  EXPECT_TRUE(mutator->HasAnimators());
  // No memory - the previous mutationis not stored.
  EXPECT_EQ(0, anim1->TestLast());
  EXPECT_EQ(0, anim1->TestCount());
  EXPECT_EQ(0, anim2->TestLast());
  EXPECT_EQ(0, anim2->TestCount());

  mutator->Mutate(testvalue1);
  EXPECT_EQ(testvalue1, anim1->TestLast());
  EXPECT_EQ(1, anim1->TestCount());
  EXPECT_EQ(0, anim2->TestLast());
  EXPECT_EQ(0, anim2->TestCount());

  mutator->RegisterCompositorAnimator(anim2);
  const double testvalue2 = 2.78;
  mutator->Mutate(testvalue2);
  EXPECT_EQ(testvalue2, anim1->TestLast());
  EXPECT_EQ(2, anim1->TestCount());
  EXPECT_EQ(testvalue2, anim2->TestLast());
  EXPECT_EQ(1, anim2->TestCount());

  mutator->UnregisterCompositorAnimator(anim1);
  EXPECT_TRUE(mutator->HasAnimators());

  mutator->UnregisterCompositorAnimator(anim2);
  EXPECT_FALSE(mutator->HasAnimators());

  // Make sure the Unregistered keeps them from being mutated.
  mutator->Mutate(114.321);
  EXPECT_EQ(testvalue2, anim1->TestLast());
  EXPECT_EQ(2, anim1->TestCount());
  EXPECT_EQ(testvalue2, anim2->TestLast());
  EXPECT_EQ(1, anim2->TestCount());
}

class EmptyCompositorMutatorClient final : public CompositorMutatorClient {
 public:
  const int MANY = 10000;
  explicit EmptyCompositorMutatorClient(CompositorMutator* m)
      : CompositorMutatorClient(m), notifications_(0) {}

  void NotifyMutationUpdate() override {
    if (notifications_ < MANY)
      notifications_++;
  }

  bool WasNotified() { return notifications_; }
  void ClearNotified() { notifications_ = 0; }
  int Notifications() { return notifications_; }

 private:
  int notifications_;
};

TEST_F(CompositorMutatorImplTest, AnimatorNotification) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::CreateForTest();
  EXPECT_NOT_NULL(mutator);

  EmptyCompositorMutatorClient mailbox(mutator);

  EXPECT_FALSE(mailbox.WasNotified());
  mutator->SetClient(&mailbox);
  EXPECT_FALSE(mailbox.WasNotified());

  mutator->Mutate(0);
  EXPECT_TRUE(mailbox.WasNotified());
  EXPECT_EQ(1, mailbox.Notifications());

  mutator->Mutate(0.4);
  EXPECT_TRUE(mailbox.WasNotified());
  EXPECT_EQ(2, mailbox.Notifications());

  EmptyCompositorAnimator* anim1 = new EmptyCompositorAnimator();
  EmptyCompositorAnimator* anim2 = new EmptyCompositorAnimator();
  EmptyCompositorAnimator* anim3 = new EmptyCompositorAnimator();
  mutator->RegisterCompositorAnimator(anim1);
  mutator->RegisterCompositorAnimator(anim2);
  mutator->RegisterCompositorAnimator(anim3);

  mutator->Mutate(0.4);
  EXPECT_TRUE(mailbox.WasNotified());
  EXPECT_EQ(3, mailbox.Notifications());

  mutator->Mutate(0.5);
  EXPECT_TRUE(mailbox.WasNotified());
  EXPECT_EQ(4, mailbox.Notifications());

  mailbox.ClearNotified();
  EXPECT_FALSE(mailbox.WasNotified());

  mutator->UnregisterCompositorAnimator(anim1);
  mutator->Mutate(0.2);
  EXPECT_TRUE(mailbox.WasNotified());

  mailbox.ClearNotified();
  mutator->SetClient((CompositorMutatorClient*)0);
  EXPECT_FALSE(mailbox.WasNotified());
  mutator->Mutate(0.2);
  EXPECT_FALSE(mailbox.WasNotified());
  mutator->Mutate(0.9);
  EXPECT_FALSE(mailbox.WasNotified());
}

}  // namespace blink
