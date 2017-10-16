// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CompositorMutatorImpl.h"
#include "platform/graphics/CompositorMutatorClient.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

#define EXPECT_NOT_NULL(p) EXPECT_TRUE(p)

TEST(CompositorMutatorImplTest, Creation) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::Create();

  EXPECT_NOT_NULL(mutator);

  mutator = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator);

  CompositorMutatorImpl* mutator2 = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator2);
  EXPECT_NE(mutator, mutator2);
}

namespace {

// This is a manual mock Client that just recieves and counts invocations.
class TestLayerMutatorClient final : public cc::LayerTreeMutatorClient {
 public:
  const int MANY = 10000;
  explicit TestLayerMutatorClient(CompositorMutator* mutator)
      : peer_(mutator), notifications_(0) {
    peer_.SetClient(this);
  }

  CompositorMutatorClient* GetCompositorMutatorClient() { return &peer_; }

  void SetNeedsMutate() override {
    if (notifications_ < MANY)
      notifications_++;
  }

  bool WasNotified() { return notifications_; }
  void ClearNotified() { notifications_ = 0; }
  int Notifications() { return notifications_; }

 private:
  CompositorMutatorClient peer_;
  int notifications_;
};

// This is a manual mock Animator that just recieves and counts invocations,
// and keeps the last paylod for checking.
class TestCompositorAnimator final
    : public GarbageCollected<TestCompositorAnimator>,
      public CompositorAnimator {
  USING_GARBAGE_COLLECTED_MIXIN(TestCompositorAnimator);

 public:
  bool Mutate(double payload) override {
    last_payload_ = payload;
    mutation_count_++;
    return true;
  }

  // What has been done to you?
  double TestLast() const { return last_payload_; };
  int TestCount() const { return mutation_count_; };

  TestCompositorAnimator() : last_payload_(0), mutation_count_(0) {}

 private:
  double last_payload_;
  int mutation_count_;
};

}  // Anonymous namespace

// The test class is a little complicated, let's make sure it is
// working as expected.  So we test the test.
TEST(CompositorMutatorImplTest, TestTestLayerMutatorClient) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator);

  TestLayerMutatorClient* client = new TestLayerMutatorClient(mutator);
  // Initial conditions are weird since we are so tied up in construction.
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());

  client->ClearNotified();
  EXPECT_FALSE(client->WasNotified());
  EXPECT_EQ(0, client->Notifications());

  client->SetNeedsMutate();
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());

  client->SetNeedsMutate();
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(2, client->Notifications());

  client->ClearNotified();
  EXPECT_FALSE(client->WasNotified());
  EXPECT_EQ(0, client->Notifications());

  client->SetNeedsMutate();
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());

  client->ClearNotified();
  EXPECT_FALSE(client->WasNotified());
  EXPECT_EQ(0, client->Notifications());

  client->GetCompositorMutatorClient()->SetNeedsMutate();
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());
}

TEST(CompositorMutatorImplTest, RegisterClient) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::Create();
  ASSERT_TRUE(mutator);

  TestLayerMutatorClient* client = new TestLayerMutatorClient(mutator);
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());
}

TEST(CompositorMutatorImplTest, AnimatorMutation) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator);

  CompositorMutatorImpl* dummy = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator);

  // CompositorMutator is broken if we try to mutate without a client.
  // We can't create a client without setting it in some mutator.
  TestLayerMutatorClient* client = new TestLayerMutatorClient(dummy);
  EXPECT_TRUE(client->WasNotified());
  client->ClearNotified();

  mutator->SetClient(client->GetCompositorMutatorClient());
  EXPECT_FALSE(client->WasNotified());

  TestCompositorAnimator* anim1 = new TestCompositorAnimator;
  TestCompositorAnimator* anim2 = new TestCompositorAnimator;
  EXPECT_EQ(0, anim1->TestLast());
  EXPECT_EQ(0, anim1->TestCount());
  EXPECT_EQ(0, anim2->TestLast());
  EXPECT_EQ(0, anim2->TestCount());

  // Mutation with no animators - OK, and effect-free.
  const double testvalue1 = 3.1416;
  // No mutators, no reason to mutate again.
  EXPECT_FALSE(mutator->Mutate(testvalue1));
  EXPECT_EQ(0, anim1->TestLast());
  EXPECT_EQ(0, anim1->TestCount());

  // Mutation with a single animator registered.
  mutator->RegisterCompositorAnimator(anim1);
  EXPECT_TRUE(client->WasNotified());

  // No memory - the previous mutation is not stored.
  EXPECT_EQ(0, anim1->TestLast());
  EXPECT_EQ(0, anim1->TestCount());
  EXPECT_EQ(0, anim2->TestLast());
  EXPECT_EQ(0, anim2->TestCount());

  EXPECT_TRUE(mutator->Mutate(testvalue1));
  EXPECT_EQ(testvalue1, anim1->TestLast());
  EXPECT_EQ(1, anim1->TestCount());
  EXPECT_EQ(0, anim2->TestLast());
  EXPECT_EQ(0, anim2->TestCount());
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());
  client->ClearNotified();

  mutator->RegisterCompositorAnimator(anim2);
  const double testvalue2 = 2.78;
  EXPECT_TRUE(mutator->Mutate(testvalue2));
  EXPECT_EQ(testvalue2, anim1->TestLast());
  EXPECT_EQ(2, anim1->TestCount());
  EXPECT_EQ(testvalue2, anim2->TestLast());
  EXPECT_EQ(1, anim2->TestCount());
  // No matter how many animators, 1 NeedsMutation.
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());

  mutator->UnregisterCompositorAnimator(anim1);
  mutator->UnregisterCompositorAnimator(anim2);

  // Make sure the Unregistered animators are not mutated.
  EXPECT_FALSE(mutator->Mutate(114.321));
  EXPECT_EQ(testvalue2, anim1->TestLast());
  EXPECT_EQ(2, anim1->TestCount());
  EXPECT_EQ(testvalue2, anim2->TestLast());
  EXPECT_EQ(1, anim2->TestCount());
}
}  // namespace blink
