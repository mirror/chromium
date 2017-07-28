// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/location_bar.h"

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

namespace {

gfx::Point3F GetPosition(UiElement* element) {
  gfx::Point3F position;
  element->LocalTransform().TransformPoint(&position);
  return position;
}

}  // namespace

class LocationBarTest : public testing::Test {
 public:
  void SetUp() override {
    background_element_ = base::MakeUnique<UiElement>();
    background_element_->SetSize(1.0f, 1.0f);
    thumb_element_ = base::MakeUnique<UiElement>();
    thumb_element_->SetSize(9.0f, 5.0f);
    location_bar_ = base::MakeUnique<LocationBar>(background_element_.get(),
                                                  thumb_element_.get());
    location_bar_->SetSize(30.0f, 10.0f);
  }

 protected:
  std::unique_ptr<UiElement> background_element_;
  std::unique_ptr<UiElement> thumb_element_;
  std::unique_ptr<LocationBar> location_bar_;
};

TEST_F(LocationBarTest, InitialLayout) {
  EXPECT_FLOAT_EQ(background_element_->size().width(), 30.0f);
  EXPECT_FLOAT_EQ(background_element_->size().height(), 10.0f);
  EXPECT_FLOAT_EQ(thumb_element_->size().width(), 9.0f);
  EXPECT_FLOAT_EQ(thumb_element_->size().height(), 5.0f);
  EXPECT_FLOAT_EQ(location_bar_->size().width(), 30.0f);
  EXPECT_FLOAT_EQ(location_bar_->size().height(), 10.0f);

  auto thumb_element__position = GetPosition(thumb_element_.get());
  EXPECT_FLOAT_EQ(thumb_element__position.x(), 0.0f);
  EXPECT_FLOAT_EQ(thumb_element__position.y(), 0.0f);
  EXPECT_FLOAT_EQ(thumb_element__position.z(), 0.0f);

  EXPECT_EQ(background_element_->parent(), location_bar_.get());
  EXPECT_EQ(thumb_element_->parent(), location_bar_.get());
}

TEST_F(LocationBarTest, SetRange) {
  location_bar_->SetRange(4);

  auto thumb_element__position = GetPosition(thumb_element_.get());
  EXPECT_FLOAT_EQ(thumb_element__position.x(), -11.25);
  EXPECT_FLOAT_EQ(thumb_element__position.y(), 0.0f);
  EXPECT_FLOAT_EQ(thumb_element__position.z(), 0.0f);
}

TEST_F(LocationBarTest, SetPosition) {
  location_bar_->SetRange(4);
  location_bar_->SetPosition(1);

  auto thumb_element__position = GetPosition(thumb_element_.get());
  EXPECT_FLOAT_EQ(thumb_element__position.x(), -3.75f);
  EXPECT_FLOAT_EQ(thumb_element__position.y(), 0.0f);
  EXPECT_FLOAT_EQ(thumb_element__position.z(), 0.0f);
}

}  // namespace vr
