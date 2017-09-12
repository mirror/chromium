// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/resolver/StyleResolver.h"
#include "core/dom/Document.h"
#include "core/dom/ShadowRoot.h"
#include "core/html/HTMLDivElement.h"
#include "core/html/HTMLStyleElement.h"
#include "core/loader/EmptyClients.h"
#include "core/style/ComputedStyle.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/testing/UnitTestHelpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class StyleResolverTest : public ::testing::Test {
 protected:
  virtual void SetUp() { InitializePage(); }

  void InitializePage() {
    Page::PageClients clients;
    FillWithEmptyClients(clients);
    page_holder_ = DummyPageHolder::Create(IntSize(800, 600), &clients,
                                           EmptyLocalFrameClient::Create());

    GetDocument().write("<div id=\"root\">");
  }

  // Generate the following DOM structure and return the innermost <div>.
  //  + div#root
  //    + #shadow
  //      + div pseudo="shadowId"
  //      |  + #shadow
  //      |    + div class="test"
  //      + style
  Element* InitializeShadowDOMTree(const AtomicString& shadow_id) {
    Element* root = GetDocument().getElementById("root");
    ShadowRoot* first_shadow = root->CreateShadowRootInternal(
        ShadowRootType::kUserAgent, ASSERT_NO_EXCEPTION);

    HTMLDivElement* psuedo_div = HTMLDivElement::Create(GetDocument());
    psuedo_div->SetShadowPseudoId(shadow_id);
    first_shadow->AppendChild(psuedo_div);
    ShadowRoot* second_shadow = psuedo_div->CreateShadowRootInternal(
        ShadowRootType::kUserAgent, ASSERT_NO_EXCEPTION);

    HTMLDivElement* class_div = HTMLDivElement::Create(GetDocument());
    class_div->setAttribute("class", "test");
    second_shadow->AppendChild(class_div);

    HTMLStyleElement* style = HTMLStyleElement::Create(GetDocument(), false);
    style->setInnerText(".test { background: red; }", ASSERT_NO_EXCEPTION);
    first_shadow->AppendChild(style);

    return class_div;
  }

  Document& GetDocument() { return page_holder_->GetDocument(); }

  bool HasMediaControlAncestor(const Element* element) {
    return StyleResolver::HasMediaControlAncestor(element);
  }

 private:
  std::unique_ptr<DummyPageHolder> page_holder_;
};

TEST_F(StyleResolverTest, HasMediaControlAncestor_Fail) {
  EXPECT_FALSE(
      HasMediaControlAncestor(InitializeShadowDOMTree("-webkit-test")));
}

TEST_F(StyleResolverTest, HasMediaControlAncestor_WebKit) {
  EXPECT_TRUE(HasMediaControlAncestor(
      InitializeShadowDOMTree("-webkit-media-controls")));
}

TEST_F(StyleResolverTest, HasMediaControlAncestor_Internal) {
  EXPECT_TRUE(HasMediaControlAncestor(
      InitializeShadowDOMTree("-internal-media-controls")));
}

}  // namespace blink
