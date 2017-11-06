// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/page/SpatialNavigation.h"

#include "core/frame/FrameTestHelpers.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/html/HTMLElement.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "platform/testing/RuntimeEnabledFeaturesTestHelpers.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/Platform.h"
#include "public/platform/WebURLLoaderMockFactory.h"
#include "public/web/WebSettings.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class SpatialNavigationTest : public ::testing::Test {
 public:
  SpatialNavigationTest() : base_url_("http://www.test.com/") {
    RegisterMockedHttpURLLoad("root-scroller-iframe.html");
    RegisterMockedHttpURLLoad("root-scroller-child.html");
  }

  ~SpatialNavigationTest() override {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  WebViewImpl* Initialize() {
    return InitializeInternal("about:blank");
  }

  WebViewImpl* Initialize(const std::string& page_name) {
    return InitializeInternal(base_url_ + page_name);
  }

  static void ConfigureSettings(WebSettings* settings) {
    settings->SetJavaScriptEnabled(true);
  }

  void RegisterMockedHttpURLLoad(const std::string& file_name) {
    URLTestHelpers::RegisterMockedURLLoadFromBase(
        WebString::FromUTF8(base_url_), testing::CoreTestDataPath(),
        WebString::FromUTF8(file_name));
  }

  WebViewImpl* GetWebView() const {
    return helper_.WebView();
  }

  LocalFrame* MainFrame() const {
    return GetWebView()->MainFrameImpl()->GetFrame();
  }

 private:
  WebViewImpl* InitializeInternal(const std::string& url) {
    // Load a page with large body and set viewport size to 400x400 to ensure
    // main frame is scrollable.
    helper_.InitializeAndLoad(url, nullptr, nullptr, nullptr,
                              &ConfigureSettings);
    GetWebView()->Resize(IntSize(400, 400));
    return GetWebView();
  }

  std::string base_url_;
  FrameTestHelpers::WebViewHelper helper_;
};

TEST_F(SpatialNavigationTest, FindContainerWhenEnclosingContainerIsDocument) {
  Initialize();

  WebURL base_url = URLTestHelpers::ToKURL("http://www.test.com/");
  FrameTestHelpers::LoadHTMLString(GetWebView()->MainFrameImpl(),
                                   "<!DOCTYPE html>"
                                   "<div id='child'></div>",
                                   base_url);

  Element* child_element = MainFrame()->GetDocument()->getElementById("child");

  Node* enclosing_container =
      ScrollableEnclosingBoxOrParentFrameForNodeInDirection(
          WebFocusType::kWebFocusTypeDown, child_element);

  EXPECT_EQ(enclosing_container, MainFrame()->GetDocument());
}

TEST_F(SpatialNavigationTest, FindContainerWhenEnclosingContainerIsIframe) {
  // Document: root-scroller-iframe.html
  //    |
  //  body
  //    |
  // iframe: id=iframe, src=root-scroller-child.html
  //    |
  //  body
  //    |
  //   div: id=container
  Initialize("root-scroller-iframe.html");

  HTMLFrameOwnerElement* iframe = ToHTMLFrameOwnerElement(
      MainFrame()->GetDocument()->getElementById("iframe"));
  Element* inner_container_element =
      iframe->contentDocument()->getElementById("container");

  Node* enclosing_container =
      ScrollableEnclosingBoxOrParentFrameForNodeInDirection(
          WebFocusType::kWebFocusTypeDown, inner_container_element);

  EXPECT_EQ(enclosing_container, iframe->contentDocument());
}

TEST_F(SpatialNavigationTest,
       FindContainerWhenEnclosingContainerIsScrollableOverflowBox) {
  Initialize();

  WebURL base_url = URLTestHelpers::ToKURL("http://www.test.com/");
  FrameTestHelpers::LoadHTMLString(GetWebView()->MainFrameImpl(),
                                   "<!DOCTYPE html>"
                                   "<style>"
                                   "  body, html {"
                                   "    height: 100%;"
                                   "    margin: 0px;"
                                   "  }"
                                   "  #child {"
                                   "    height: 1000px;"
                                   "  }"
                                   "  #container {"
                                   "    width: 100%;"
                                   "    height: 100%;"
                                   "    overflow-y: scroll;"
                                   "  }"
                                   "</style>"
                                   "<div id='container'>"
                                   "  <div id='child'></div>"
                                   "</div>",
                                   base_url);

  {
    // <div id='container'> should be selected as container because
    // it is scrollable in direction.

    Element* child_element = MainFrame()->GetDocument()
        ->getElementById("child");
    Element* container_element = MainFrame()->GetDocument()
        ->getElementById("container");

    Node* enclosing_container =
        ScrollableEnclosingBoxOrParentFrameForNodeInDirection(
            WebFocusType::kWebFocusTypeDown, child_element);

    EXPECT_EQ(enclosing_container, container_element);
  }

  {
    // In following case, <div id='container'> cannot be container
    // because it is not scrollable in direction.

    Element* child_element = MainFrame()->GetDocument()
        ->getElementById("child");

    Node* enclosing_container =
        ScrollableEnclosingBoxOrParentFrameForNodeInDirection(
            WebFocusType::kWebFocusTypeUp, child_element);
    EXPECT_EQ(enclosing_container, MainFrame()->GetDocument());

    enclosing_container =
        ScrollableEnclosingBoxOrParentFrameForNodeInDirection(
            WebFocusType::kWebFocusTypeLeft, child_element);

    EXPECT_EQ(enclosing_container, MainFrame()->GetDocument());

    enclosing_container =
        ScrollableEnclosingBoxOrParentFrameForNodeInDirection(
            WebFocusType::kWebFocusTypeRight, child_element);

    EXPECT_EQ(enclosing_container, MainFrame()->GetDocument());
  }
}

}  // namespace blink
