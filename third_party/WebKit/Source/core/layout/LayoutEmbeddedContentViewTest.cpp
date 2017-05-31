// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/LayoutEmbeddedContentView.h"

#include "core/html/HTMLElement.h"
#include "core/layout/ImageQualityController.h"
#include "core/layout/LayoutTestHelper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class LayoutEmbeddedContentViewTest : public RenderingTest {};

class OverriddenLayoutEmbeddedContentView : public LayoutEmbeddedContentView {
 public:
  explicit OverriddenLayoutEmbeddedContentView(Element* element)
      : LayoutEmbeddedContentView(element) {}

  const char* GetName() const override {
    return "OverriddenLayoutEmbeddedContentView";
  }
};

TEST_F(LayoutEmbeddedContentViewTest, DestroyUpdatesImageQualityController) {
  Element* element = HTMLElement::Create(HTMLNames::divTag, GetDocument());
  LayoutObject* part = new OverriddenLayoutEmbeddedContentView(element);
  // The third and forth arguments are not important in this test.
  ImageQualityController::GetImageQualityController()->Set(
      *part, 0, this, LayoutSize(1, 1), false);
  EXPECT_TRUE(ImageQualityController::Has(*part));
  part->Destroy();
  EXPECT_FALSE(ImageQualityController::Has(*part));
}

}  // namespace blink
