// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_offset_mapping_annotation.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class NGOffsetMappingAnnotationTest : public ::testing::Test {};

TEST_F(NGOffsetMappingAnnotationTest, NullAnnotation) {
  RefPtr<NGOffsetMappingAnnotation> null_annotation =
      NGNullAnnotation::GetInstance();

  EXPECT_TRUE(null_annotation->IsNull());
  EXPECT_FALSE(null_annotation->IsNode());
  EXPECT_FALSE(null_annotation->IsIndex());
  EXPECT_FALSE(null_annotation->IsNodeIndexPair());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeAnnotation) {
  RefPtr<NGOffsetMappingAnnotation> node_annotation =
      AdoptRef<NGOffsetMappingAnnotation>(new NGNodeAnnotation(nullptr));

  EXPECT_FALSE(node_annotation->IsNull());
  EXPECT_TRUE(node_annotation->IsNode());
  EXPECT_FALSE(node_annotation->IsIndex());
  EXPECT_FALSE(node_annotation->IsNodeIndexPair());

  EXPECT_EQ(nullptr, ToNGNodeAnnotation(node_annotation.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, IndexAnnotation) {
  RefPtr<NGOffsetMappingAnnotation> index_annotation =
      AdoptRef<NGOffsetMappingAnnotation>(new NGIndexAnnotation(0u));

  EXPECT_FALSE(index_annotation->IsNull());
  EXPECT_FALSE(index_annotation->IsNode());
  EXPECT_TRUE(index_annotation->IsIndex());
  EXPECT_FALSE(index_annotation->IsNodeIndexPair());

  EXPECT_EQ(0u, ToNGIndexAnnotation(index_annotation.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeIndexPairAnnotation) {
  RefPtr<NGOffsetMappingAnnotation> node_index_pair_annotation =
      AdoptRef<NGOffsetMappingAnnotation>(
          new NGNodeIndexPairAnnotation({nullptr, 0u}));

  EXPECT_FALSE(node_index_pair_annotation->IsNull());
  EXPECT_FALSE(node_index_pair_annotation->IsNode());
  EXPECT_FALSE(node_index_pair_annotation->IsIndex());
  EXPECT_TRUE(node_index_pair_annotation->IsNodeIndexPair());

  NGOffsetMappingAnnotation::NodeIndexPair expected(nullptr, 0u);
  EXPECT_EQ(expected,
            ToNGNodeIndexPairAnnotation(node_index_pair_annotation.Get())
                ->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NullCompositeNull) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNull());
}

TEST_F(NGOffsetMappingAnnotationTest, NullCompositeNode) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGNodeAnnotation(nullptr));
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNode());
  EXPECT_EQ(nullptr, ToNGNodeAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NullCompositeIndex) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGIndexAnnotation(0u));
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsIndex());
  EXPECT_EQ(0u, ToNGIndexAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NullCompositeNodeIndexPair) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(
          new NGNodeIndexPairAnnotation({nullptr, 0u}));
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNodeIndexPair());
  NGOffsetMappingAnnotation::NodeIndexPair expected(nullptr, 0u);
  EXPECT_EQ(expected, ToNGNodeIndexPairAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeCompositeNull) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGNodeAnnotation(nullptr));
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNode());
  EXPECT_EQ(nullptr, ToNGNodeAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeCompositeIndex) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGNodeAnnotation(nullptr));
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGIndexAnnotation(0u));
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNodeIndexPair());
  NGOffsetMappingAnnotation::NodeIndexPair expected(nullptr, 0u);
  EXPECT_EQ(expected, ToNGNodeIndexPairAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, IndexCompositeNull) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGIndexAnnotation(0u));
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsIndex());
  EXPECT_EQ(0u, ToNGIndexAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, IndexCompositeIndex) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGIndexAnnotation(0u));
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGIndexAnnotation(1u));
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsIndex());
  EXPECT_EQ(1u, ToNGIndexAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeIndexPairCompositeNull) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(
          new NGNodeIndexPairAnnotation({nullptr, 0u}));
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNodeIndexPair());
  NGOffsetMappingAnnotation::NodeIndexPair expected(nullptr, 0u);
  EXPECT_EQ(expected, ToNGNodeIndexPairAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeIndexPairCompositeIndex) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(
          new NGNodeIndexPairAnnotation({nullptr, 0u}));
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGIndexAnnotation(1u));
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNodeIndexPair());
  NGOffsetMappingAnnotation::NodeIndexPair expected(nullptr, 1u);
  EXPECT_EQ(expected, ToNGNodeIndexPairAnnotation(result.Get())->GetValue());
}

}  // namespace blink
