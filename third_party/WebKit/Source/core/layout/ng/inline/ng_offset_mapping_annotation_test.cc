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
  EXPECT_TRUE(null_annotation->IsNullAnnotation());
  EXPECT_FALSE(null_annotation->IsNodeAnnotation());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeAnnotation) {
  RefPtr<NGOffsetMappingAnnotation> node_annotation =
      AdoptRef<NGOffsetMappingAnnotation>(new NGNodeAnnotation(nullptr));
  EXPECT_FALSE(node_annotation->IsNullAnnotation());
  EXPECT_TRUE(node_annotation->IsNodeAnnotation());
  EXPECT_EQ(nullptr, ToNGNodeAnnotation(node_annotation.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NullCompositeNull) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_TRUE(result->IsNullAnnotation());
  EXPECT_FALSE(result->IsNodeAnnotation());
}

TEST_F(NGOffsetMappingAnnotationTest, NullCompositeNode) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGNodeAnnotation(nullptr));
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_FALSE(result->IsNullAnnotation());
  EXPECT_TRUE(result->IsNodeAnnotation());
  EXPECT_EQ(nullptr, ToNGNodeAnnotation(result.Get())->GetValue());
}

TEST_F(NGOffsetMappingAnnotationTest, NodeCompositeNull) {
  RefPtr<NGOffsetMappingAnnotation> left_hand_side =
      AdoptRef<NGOffsetMappingAnnotation>(new NGNodeAnnotation(nullptr));
  RefPtr<NGOffsetMappingAnnotation> right_hand_side =
      NGNullAnnotation::GetInstance();
  RefPtr<NGOffsetMappingAnnotation> result =
      left_hand_side->Composite(*right_hand_side);

  EXPECT_FALSE(result->IsNullAnnotation());
  EXPECT_TRUE(result->IsNodeAnnotation());
  EXPECT_EQ(nullptr, ToNGNodeAnnotation(result.Get())->GetValue());
}

}  // namespace blink
