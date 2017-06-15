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

  using PaintPropertyNode::SetChanged;

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
    root = MockPaintPropertyNode::Create(nullptr);
    a1_referenced = MockPaintPropertyNode::Create(nullptr);
    a1 = MockPaintPropertyNode::Create(root, a1_referenced);
    a2 = MockPaintPropertyNode::Create(root);
    b1 = MockPaintPropertyNode::Create(a1);

    //                   root
    //                   /  \
    // a1_referenced <- a1  a2
    //                 /
    //                b1
  }

  RefPtr<MockPaintPropertyNode> root;
  RefPtr<MockPaintPropertyNode> a1;
  RefPtr<MockPaintPropertyNode> a2;
  RefPtr<MockPaintPropertyNode> b1;
  RefPtr<MockPaintPropertyNode> a1_referenced;
};

TEST_F(PaintPropertyNodeTest, InitialState) {
  EXPECT_FALSE(root->Changed());
  EXPECT_FALSE(a1->Changed());
  EXPECT_FALSE(a2->Changed());
  EXPECT_FALSE(b1->Changed());
  EXPECT_FALSE(a1_referenced->Changed());
}

TEST_F(PaintPropertyNodeTest, RootChanged) {
  root->SetChanged();
  EXPECT_TRUE(root->Changed());
  EXPECT_TRUE(a1->Changed());
  EXPECT_TRUE(a2->Changed());
  EXPECT_TRUE(b1->Changed());
  EXPECT_FALSE(a1_referenced->Changed());
}

TEST_F(PaintPropertyNodeTest, ChangedInOneSubtree) {
  a1->SetChanged();
  EXPECT_FALSE(root->Changed());
  EXPECT_TRUE(a1->Changed());
  EXPECT_FALSE(a2->Changed());
  EXPECT_TRUE(b1->Changed());
  EXPECT_FALSE(a1_referenced->Changed());
}

TEST_F(PaintPropertyNodeTest, ReferencedChanged) {
  a1_referenced->SetChanged();
  EXPECT_FALSE(root->Changed());
  EXPECT_TRUE(a1->Changed());
  EXPECT_FALSE(a2->Changed());
  EXPECT_TRUE(b1->Changed());
  EXPECT_TRUE(a1_referenced->Changed());
}

}  // namespace blink
