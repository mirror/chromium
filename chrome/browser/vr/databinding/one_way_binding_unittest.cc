// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/databinding/one_way_binding.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

namespace {

struct TestModel {
  bool value;
};

}  // namespace

TEST(OneWayBinding, BoundBool) {
  TestModel a = {true};
  TestModel b = {false};

  OneWayBinding<bool> binding(
      base::Bind([](TestModel* x) { return x->value; }, base::Unretained(&a)),
      base::Bind([](TestModel* x, const bool& value) { x->value = value; },
                 base::Unretained(&b)));

  EXPECT_NE(a.value, b.value);
  binding.Update();

  EXPECT_EQ(true, a.value);
  EXPECT_EQ(true, b.value);

  a.value = false;
  EXPECT_EQ(true, b.value);

  binding.Update();
  EXPECT_EQ(false, a.value);
  EXPECT_EQ(false, b.value);

  b.value = true;
  // Since this is a one way binding, Update will not detect a change in value
  // of b. Since a's value has not changed, a new value should not be pushed to
  // b.
  binding.Update();
  EXPECT_EQ(false, a.value);
}

}  // namespace vr
