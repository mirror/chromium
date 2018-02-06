// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/core/v8/media_list_or_string.h"
#include "core/css/CSSRuleList.h"
#include "core/css/CSSStyleSheet.h"
#include "core/css/CSSStyleSheetInit.h"
#include "core/css/MediaList.h"
#include "core/testing/PageTestBase.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class CSSStyleSheetTest : public PageTestBase {
 protected:
  virtual void SetUp() {
    PageTestBase::SetUp();
    RuntimeEnabledFeatures::SetConstructableStylesheetsEnabled(true);
  }
};

TEST_F(CSSStyleSheetTest, ConstructorWithoutRuntimeFlagThrowsException) {
  DummyExceptionStateForTesting exception_state;
  RuntimeEnabledFeatures::SetConstructableStylesheetsEnabled(false);
  ASSERT_EQ(CSSStyleSheet::Create(GetDocument(), "", CSSStyleSheetInit(),
                                  exception_state),
            nullptr);
  ASSERT_TRUE(exception_state.HadException());
}

TEST_F(CSSStyleSheetTest,
       CSSStyleSheetConstructionWithEmptyCSSStyleSheetInitAndText) {
  DummyExceptionStateForTesting exception_state;
  CSSStyleSheet* sheet = CSSStyleSheet::Create(
      GetDocument(), "", CSSStyleSheetInit(), exception_state);
  ASSERT_FALSE(exception_state.HadException());
  ASSERT_TRUE(sheet->href().IsNull());
  ASSERT_EQ(sheet->parentStyleSheet(), nullptr);
  ASSERT_EQ(sheet->ownerNode(), nullptr);
  ASSERT_EQ(sheet->ownerRule(), nullptr);
  ASSERT_EQ(sheet->media()->length(), 0U);
  ASSERT_EQ(sheet->title(), StringImpl::empty_);
  ASSERT_FALSE(sheet->Alternate());
  ASSERT_FALSE(sheet->disabled());
  ASSERT_EQ(sheet->cssRules(exception_state)->length(), 0U);
  ASSERT_FALSE(exception_state.HadException());
}

TEST_F(CSSStyleSheetTest,
       CSSStyleSheetConstructionWithoutEmptyCSSStyleSheetInitAndText) {
  DummyExceptionStateForTesting exception_state;
  String styleText[2] = {".red { color: red; }",
                         ".red + span + span { color: red; }"};
  CSSStyleSheetInit init;
  init.setMedia(MediaListOrString::FromString("screen, print"));
  init.setTitle("test");
  init.setAlternate(true);
  init.setDisabled(true);
  CSSStyleSheet* sheet = CSSStyleSheet::Create(
      GetDocument(), styleText[0] + styleText[1], init, exception_state);
  ASSERT_FALSE(exception_state.HadException());
  ASSERT_TRUE(sheet->href().IsNull());
  ASSERT_EQ(sheet->parentStyleSheet(), nullptr);
  ASSERT_EQ(sheet->ownerNode(), nullptr);
  ASSERT_EQ(sheet->ownerRule(), nullptr);
  ASSERT_EQ(sheet->media()->length(), 2U);
  ASSERT_EQ(sheet->media()->mediaText(), init.media().GetAsString());
  ASSERT_EQ(sheet->title(), init.title());
  ASSERT_TRUE(sheet->Alternate());
  ASSERT_TRUE(sheet->disabled());
  ASSERT_EQ(sheet->cssRules(exception_state)->length(), 2U);
  ASSERT_EQ(sheet->cssRules(exception_state)->item(0)->cssText(), styleText[0]);
  ASSERT_EQ(sheet->cssRules(exception_state)->item(1)->cssText(), styleText[1]);
  ASSERT_FALSE(exception_state.HadException());
}

}  // namespace blink
