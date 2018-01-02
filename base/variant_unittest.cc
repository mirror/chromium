// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/variant.h"

#include "base/memory/weak_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

using VariantTest = testing::Test;

TEST_F(VariantTest, DefaultConstructor) {
  Variant<std::string, float, bool> v;

  EXPECT_EQ(0u, v.index());
  EXPECT_EQ("", (Get<0>(&v)));
}

TEST_F(VariantTest, Constructor) {
  Variant<std::string, float, bool> v0(std::string("Hello"));
  Variant<std::string, float, bool> v1(1.234f);
  Variant<std::string, float, bool> v2(true);

  EXPECT_EQ(0u, v0.index());
  EXPECT_EQ("Hello", (Get<0>(&v0)));

  EXPECT_EQ(1u, v1.index());
  EXPECT_EQ(1.234f, (Get<1>(&v1)));

  EXPECT_EQ(2u, v2.index());
  EXPECT_TRUE((Get<2>(&v2)));
}

namespace {
struct DestructorTestType {
  DestructorTestType() : weak_factory(this) {}

  WeakPtrFactory<DestructorTestType> weak_factory;
};
}  // namespace

TEST_F(VariantTest, EmplaceDestructsPreviousType) {
  Variant<DestructorTestType, bool> v;
  WeakPtr<DestructorTestType> weak_ptr =
      Get<DestructorTestType>(&v).weak_factory.GetWeakPtr();

  EXPECT_TRUE(weak_ptr);
  auto& return_value = v.emplace<bool>(true);

  EXPECT_FALSE(weak_ptr);
  EXPECT_TRUE(Get<bool>(&v));
  EXPECT_TRUE(return_value);
}

TEST_F(VariantTest, IndexEmplace) {
  Variant<std::unique_ptr<int>, std::unique_ptr<float>, std::unique_ptr<bool>>
      v;

  v.emplace(in_place_index_t<0>(), std::make_unique<int>(1234));
  EXPECT_EQ(0u, v.index());
  EXPECT_EQ(1234, (*Get<0>(&v)));

  v.emplace(in_place_index_t<1>(), std::make_unique<float>(1.234));
  EXPECT_EQ(1u, v.index());
  EXPECT_EQ(1.234f, (*Get<1>(&v)));

  v.emplace(in_place_index_t<2>(), std::make_unique<bool>(true));
  EXPECT_EQ(2u, v.index());
  EXPECT_TRUE((*Get<2>(&v)));
}

TEST_F(VariantTest, GetIf) {
  Variant<std::string, float, bool> v0(std::string("Hello"));
  Variant<std::string, float, bool> v1(1.234f);
  Variant<std::string, float, bool> v2(true);

  EXPECT_EQ(0u, v0.index());
  EXPECT_TRUE((GetIf<0>(&v0)));
  EXPECT_FALSE((GetIf<1>(&v0)));
  EXPECT_FALSE((GetIf<2>(&v0)));
  EXPECT_EQ("Hello", (*GetIf<0>(&v0)));

  EXPECT_EQ(1u, v1.index());
  EXPECT_FALSE((GetIf<0>(&v1)));
  EXPECT_TRUE((GetIf<1>(&v1)));
  EXPECT_FALSE((GetIf<2>(&v1)));
  EXPECT_EQ(1.234f, (*GetIf<1>(&v1)));

  EXPECT_EQ(2u, v2.index());
  EXPECT_FALSE((GetIf<0>(&v2)));
  EXPECT_FALSE((GetIf<1>(&v2)));
  EXPECT_TRUE((GetIf<2>(&v2)));
  EXPECT_TRUE((*GetIf<2>(&v2)));
}

TEST_F(VariantTest, AssignValue) {
  Variant<int, bool, std::string> v;

  v = 1234;
  EXPECT_TRUE((GetIf<int>(&v)));
  EXPECT_FALSE((GetIf<bool>(&v)));
  EXPECT_FALSE((GetIf<std::string>(&v)));
  EXPECT_EQ(1234, (Get<int>(&v)));

  v = true;
  EXPECT_FALSE((GetIf<int>(&v)));
  EXPECT_TRUE((GetIf<bool>(&v)));
  EXPECT_FALSE((GetIf<std::string>(&v)));
  EXPECT_TRUE((Get<bool>(&v)));

  v = std::string("hello");
  EXPECT_FALSE((GetIf<int>(&v)));
  EXPECT_FALSE((GetIf<bool>(&v)));
  EXPECT_TRUE((GetIf<std::string>(&v)));
  EXPECT_EQ("hello", (Get<std::string>(&v)));
}

TEST_F(VariantTest, AssignValueDestructsPreviousType) {
  Variant<DestructorTestType, int, std::string> v;
  WeakPtr<DestructorTestType> weak_ptr =
      Get<DestructorTestType>(&v).weak_factory.GetWeakPtr();

  EXPECT_TRUE(weak_ptr);
  v = 1234;

  EXPECT_FALSE(weak_ptr);
  EXPECT_EQ(1234, (Get<int>(&v)));
}

TEST_F(VariantTest, CopyAssign) {
  Variant<int, bool, std::string> v1(1234);
  EXPECT_TRUE((GetIf<int>(&v1)));
  EXPECT_EQ(1234, (Get<int>(&v1)));

  Variant<int, bool, std::string> v2(std::string("Text"));
  EXPECT_TRUE((GetIf<std::string>(&v2)));
  EXPECT_EQ("Text", (Get<std::string>(&v2)));

  v1 = v2;

  EXPECT_FALSE((GetIf<int>(&v1)));
  EXPECT_TRUE((GetIf<std::string>(&v1)));
  EXPECT_EQ("Text", (Get<std::string>(&v1)));
}

TEST_F(VariantTest, MoveAssign) {
  Variant<float, std::unique_ptr<int>> v1;
  Variant<float, std::unique_ptr<int>> v2(std::make_unique<int>(1234));

  EXPECT_TRUE((Get<std::unique_ptr<int>>(&v2)));
  EXPECT_EQ(1234, *(Get<std::unique_ptr<int>>(&v2)));

  v1 = std::move(v2);

  EXPECT_FALSE((Get<std::unique_ptr<int>>(&v2)));

  EXPECT_TRUE((GetIf<std::unique_ptr<int>>(&v1)));
  EXPECT_EQ(1234, *(Get<std::unique_ptr<int>>(&v1)));
}

}  // namespace base
