// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_pointer_set.h"

#include <gtest/gtest.h>

namespace crazy {

TEST(PointerSet, DefaultIsEmpty) {
  PointerSet set;
  Vector<void*> values;
  set.GetValues(&values);
  EXPECT_TRUE(values.IsEmpty());
  EXPECT_FALSE(set.Has(nullptr));
}

TEST(PointerSet, Has) {
  PointerSet set;

  // nullptr is a perfectly valid value in the set.
  EXPECT_FALSE(set.Has(nullptr));
  EXPECT_FALSE(set.Add(nullptr));
  EXPECT_TRUE(set.Has(nullptr));
}

TEST(PointerSet, Add) {
  PointerSet set;

  auto* kOne = reinterpret_cast<void*>(1);
  auto* kTwo = reinterpret_cast<void*>(2);
  auto* kTen = reinterpret_cast<void*>(10);

  EXPECT_FALSE(set.Add(kOne));
  EXPECT_TRUE(set.Add(kOne));
  EXPECT_FALSE(set.Add(kTen));

  EXPECT_TRUE(set.Has(kOne));
  EXPECT_FALSE(set.Has(kTwo));
  EXPECT_TRUE(set.Has(kTen));

  Vector<void*> values;
  set.GetValues(&values);
  EXPECT_EQ(2U, values.GetCount());
  EXPECT_EQ(kOne, values[0]);
  EXPECT_EQ(kTen, values[1]);
}

TEST(PointerSet, Remove) {
  PointerSet set;

  auto* kOne = reinterpret_cast<void*>(1);
  auto* kTwo = reinterpret_cast<void*>(2);
  auto* kTen = reinterpret_cast<void*>(10);

  EXPECT_FALSE(set.Add(kOne));
  EXPECT_FALSE(set.Add(kTwo));
  EXPECT_FALSE(set.Add(kTen));

  EXPECT_TRUE(set.Has(kOne));
  EXPECT_TRUE(set.Has(kTwo));
  EXPECT_TRUE(set.Has(kTen));

  EXPECT_TRUE(set.Remove(kTwo));

  EXPECT_TRUE(set.Has(kOne));
  EXPECT_FALSE(set.Has(kTwo));
  EXPECT_TRUE(set.Has(kTen));

  EXPECT_FALSE(set.Remove(kTwo));

  Vector<void*> values;
  set.GetValues(&values);
  EXPECT_EQ(2U, values.GetCount());
  EXPECT_EQ(kOne, values[0]);
  EXPECT_EQ(kTen, values[1]);
}

}  // namespace crazy
