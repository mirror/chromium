// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/paint/PaintPropertyNode.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class MockPaintPropertyNode : public PaintPropertyNode<MockPaintPropertyNode> {
 public:
  static PassRefPtr<MockPaintPropertyNode> Create(
      PassRefPtr<MockPaintPropertyNode> parent,
      PassRefPtr<MockPaintPropertyNode> referenced = nullptr) {
    return AdoptRef(
        new MockPaintPropertyNode(std::move(parent), std::move(referenced)));
  }

  bool ReferencedNodeChanged() const {
    return referenced_ && referenced_->Changed();
  }

  // Simulate a change without changing the parent.
  void Update() { PaintPropertyNode::Update(Parent()); }

  void UpdateWithNewParent(PassRefPtr<MockPaintPropertyNode> new_parent) {
    PaintPropertyNode::Update(std::move(new_parent));
  }

 private:
  MockPaintPropertyNode(PassRefPtr<MockPaintPropertyNode> parent,
                        PassRefPtr<MockPaintPropertyNode> referenced)
      : PaintPropertyNode(std::move(parent)),
        referenced_(std::move(referenced)) {}

  RefPtr<MockPaintPropertyNode> referenced_;
};

class PaintPropertyNodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    a = MockPaintPropertyNode::Create(MockPaintPropertyNode::Create(nullptr));
    a1_referenced =
        MockPaintPropertyNode::Create(MockPaintPropertyNode::Create(nullptr));
    a1 = MockPaintPropertyNode::Create(a, a1_referenced);
    a2 = MockPaintPropertyNode::Create(a);
    a11 = MockPaintPropertyNode::Create(a1);

    //                     a
    //                    / \
    // a1_referenced <- a1   a2
    //                 /
    //               a11
  }

  void ResetAllChangedStatus() {
    a->ResetChangedStatus();
    a1->ResetChangedStatus();
    a2->ResetChangedStatus();
    a11->ResetChangedStatus();
    a1_referenced->ResetChangedStatus();
  }

  void ExpectInitialState() {
    EXPECT_FALSE(a->Changed());
    EXPECT_FALSE(a1->Changed());
    EXPECT_FALSE(a2->Changed());
    EXPECT_FALSE(a11->Changed());
    EXPECT_FALSE(a1_referenced->Changed());
  }

  RefPtr<MockPaintPropertyNode> a;
  RefPtr<MockPaintPropertyNode> a1;
  RefPtr<MockPaintPropertyNode> a2;
  RefPtr<MockPaintPropertyNode> a11;
  RefPtr<MockPaintPropertyNode> a1_referenced;
};

TEST_F(PaintPropertyNodeTest, InitialStateAndReset) {
  ExpectInitialState();
  ResetAllChangedStatus();
  ExpectInitialState();
}

TEST_F(PaintPropertyNodeTest, Change) {
  a->Update();
  EXPECT_TRUE(a->Changed());
  EXPECT_TRUE(a1->Changed());
  EXPECT_TRUE(a2->Changed());
  EXPECT_TRUE(a11->Changed());
  EXPECT_FALSE(a1_referenced->Changed());

  ResetAllChangedStatus();
  ExpectInitialState();
}

TEST_F(PaintPropertyNodeTest, PartialChangeAndPartialReset) {
  a1->Update();
  EXPECT_FALSE(a->Changed());
  EXPECT_TRUE(a1->Changed());
  EXPECT_FALSE(a2->Changed());
  EXPECT_TRUE(a11->Changed());
  EXPECT_FALSE(a1_referenced->Changed());

  a11->ResetChangedStatus();
  EXPECT_TRUE(a11->Changed());

  a11->ResetChangedStatus();
  a1->ResetChangedStatus();
  EXPECT_FALSE(a11->Changed());
  EXPECT_FALSE(a1->Changed());

  ResetAllChangedStatus();
  ExpectInitialState();
}

TEST_F(PaintPropertyNodeTest, ReferencedChanged) {
  a1_referenced->Update();
  EXPECT_FALSE(a->Changed());
  EXPECT_TRUE(a1->Changed());
  EXPECT_FALSE(a2->Changed());
  EXPECT_TRUE(a11->Changed());
  EXPECT_TRUE(a1_referenced->Changed());

  ResetAllChangedStatus();
  ExpectInitialState();
}

TEST_F(PaintPropertyNodeTest, Reparent) {
  a1->UpdateWithNewParent(a2);
  EXPECT_FALSE(a->Changed());
  EXPECT_TRUE(a1->Changed());
  EXPECT_FALSE(a2->Changed());
  EXPECT_TRUE(a11->Changed());
  EXPECT_FALSE(a1_referenced->Changed());

  ResetAllChangedStatus();
  ExpectInitialState();
}

}  // namespace blink
