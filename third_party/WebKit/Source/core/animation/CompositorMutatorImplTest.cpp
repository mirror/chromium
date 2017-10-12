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

  mutator = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator);

  CompositorMutatorImpl* mutator2 = CompositorMutatorImpl::Create();
  EXPECT_NOT_NULL(mutator2);
  EXPECT_NE(mutator, mutator2);
}

class EmptyLayerMutatorClient final : public cc::LayerTreeMutatorClient {
 public:
  const int MANY = 10000;
  explicit EmptyLayerMutatorClient(CompositorMutator* m)
      : cmc_(m), notifications_(0) {
    cmc_.SetClient(this);
  }

  CompositorMutatorClient* GetCompositorMutatorClient() { return &cmc_; }

  void SetNeedsMutate() override {
    if (notifications_ < MANY)
      notifications_++;
  }

  bool WasNotified() { return notifications_; }
  void ClearNotified() { notifications_ = 0; }
  int Notifications() { return notifications_; }

 private:
  CompositorMutatorClient cmc_;
  int notifications_;
};

class EmptyCompositorAnimator final : public CompositorAnimator {
 public:
  bool Mutate(double m) {
    now_ = m;
    count_++;
    return true;
  }

  // What has been done to you?
  double TestLast() const { return now_; };
  int TestCount() const { return count_; };

  EmptyCompositorAnimator() : now_(0), count_(0) {}

  // And neuter the Garbage Collection stuff.
  void AdjustAndMark(Visitor*) const override {}
  HeapObjectHeader* GetHeapObjectHeader() const { return NULL; }
  void AdjustAndTraceMarkedWrapper(
      const ScriptWrappableVisitor*) const override {}

 private:
  double now_;
  int count_;
};

TEST_F(CompositorMutatorImplTest, TestTestEmptyLayerMutatorClient) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::CreateForTest();
  EXPECT_NOT_NULL(mutator);

  EmptyLayerMutatorClient* client = new EmptyLayerMutatorClient(mutator);
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

TEST_F(CompositorMutatorImplTest, RegisterClient) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::CreateForTest();
  EXPECT_NOT_NULL(mutator);

  EmptyLayerMutatorClient* client = new EmptyLayerMutatorClient(mutator);
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());
}

TEST_F(CompositorMutatorImplTest, AnimatorMutation) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::CreateForTest();
  EXPECT_NOT_NULL(mutator);

  CompositorMutatorImpl* dummy = CompositorMutatorImpl::CreateForTest();
  EXPECT_NOT_NULL(mutator);

  // CompositorMutator is broken if we try to mutate without a client.
  // We can't create a client without setting it in some mutator.
  EmptyLayerMutatorClient* client = new EmptyLayerMutatorClient(dummy);
  EXPECT_TRUE(client->WasNotified());
  client->ClearNotified();

  mutator->SetClient(client->GetCompositorMutatorClient());
  EXPECT_FALSE(client->WasNotified());

  EmptyCompositorAnimator anim1, anim2;
  EXPECT_EQ(0, anim1.TestLast());
  EXPECT_EQ(0, anim1.TestCount());
  EXPECT_EQ(0, anim2.TestLast());
  EXPECT_EQ(0, anim2.TestCount());

  // Mutation with no animators - OK, and effect-free.
  const double testvalue1 = 3.1416;
  // No mutators, no reason to mutate again.
  EXPECT_FALSE(mutator->Mutate(testvalue1));
  EXPECT_EQ(0, anim1.TestLast());
  EXPECT_EQ(0, anim1.TestCount());

  // Mutation with a single animator registered.
  mutator->RegisterCompositorAnimator(&anim1);
  EXPECT_TRUE(client->WasNotified());

  // No memory - the previous mutation is not stored.
  EXPECT_EQ(0, anim1.TestLast());
  EXPECT_EQ(0, anim1.TestCount());
  EXPECT_EQ(0, anim2.TestLast());
  EXPECT_EQ(0, anim2.TestCount());

  EXPECT_TRUE(mutator->Mutate(testvalue1));
  EXPECT_EQ(testvalue1, anim1.TestLast());
  EXPECT_EQ(1, anim1.TestCount());
  EXPECT_EQ(0, anim2.TestLast());
  EXPECT_EQ(0, anim2.TestCount());
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());
  client->ClearNotified();

  mutator->RegisterCompositorAnimator(&anim2);
  const double testvalue2 = 2.78;
  EXPECT_TRUE(mutator->Mutate(testvalue2));
  EXPECT_EQ(testvalue2, anim1.TestLast());
  EXPECT_EQ(2, anim1.TestCount());
  EXPECT_EQ(testvalue2, anim2.TestLast());
  EXPECT_EQ(1, anim2.TestCount());
  // No matter how many animators, 1 NeedsMutation.
  EXPECT_TRUE(client->WasNotified());
  EXPECT_EQ(1, client->Notifications());

  mutator->UnregisterCompositorAnimator(&anim1);
  mutator->UnregisterCompositorAnimator(&anim2);

  // Make sure the Unregistered animators are not mutated.
  EXPECT_FALSE(mutator->Mutate(114.321));
  EXPECT_EQ(testvalue2, anim1.TestLast());
  EXPECT_EQ(2, anim1.TestCount());
  EXPECT_EQ(testvalue2, anim2.TestLast());
  EXPECT_EQ(1, anim2.TestCount());
}
}  // namespace blink
