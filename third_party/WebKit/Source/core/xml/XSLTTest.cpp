// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/FrameTestHelpers.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "core/xml/DocumentXSLT.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/Platform.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURLLoaderMockFactory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

using URLTestHelpers::ToKURL;

class XSLTTest : public ::testing::Test {
 protected:
  void SetUp() override { helper_.Initialize(); }

  void TearDown() override {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  void RegisterMockedURLLoad(const std::string& url,
                             const std::string& file_name) {
    URLTestHelpers::RegisterMockedURLLoad(
        ToKURL(url),
        testing::CoreTestDataPath(WebString::FromUTF8("xml/" + file_name)),
        WebString::FromUTF8("text/xml"));
  }

  WebLocalFrameImpl* MainFrame() const {
    return helper_.LocalMainFrame();
  }

 private:
  FrameTestHelpers::WebViewHelper helper_;
};

// Tests that verify that a transformed document inherits security properties
// from the original source document.
TEST_F(XSLTTest, TransformHttpsSecurity) {
  const char kURL[] = "https://example.com/xslt.xml";

  RegisterMockedURLLoad(kURL, "xslt.xml");
  FrameTestHelpers::LoadFrame(MainFrame(), kURL);

  Document* transformed_document = MainFrame()->GetFrame()->GetDocument();
  Document* source_document =
      DocumentXSLT::From(*transformed_document).TransformSourceDocument();
  // Intentionally use strict equality here rather than CanAccess(), to check
  // that the origin is aliased from the original document.
  EXPECT_EQ(source_document->GetSecurityOrigin(),
            transformed_document->GetSecurityOrigin());
  EXPECT_EQ(source_document->CookieURL(), transformed_document->CookieURL());
  EXPECT_TRUE(transformed_document->GetContentSecurityPolicy()->Subsumes(
      *source_document->GetContentSecurityPolicy()));
  EXPECT_TRUE(source_document->GetContentSecurityPolicy()->Subsumes(
      *transformed_document->GetContentSecurityPolicy()));
}

TEST_F(XSLTTest, TransformDataSecurity) {
  const char kURL[] = R"(data:text/xml,<?xml version="1.0" encoding="UTF-8"?>
      <?xml-stylesheet type="text/xml" href="#stylesheet"?>
      <!DOCTYPE catalog [
      <!ATTLIST xsl:stylesheet
        id    ID  #REQUIRED>
      ]>
      <root>
      <xsl:stylesheet id="stylesheet" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
      <xsl:template match="/">
      <html>
      <body>
      </body>
      </html>
      </xsl:template>
      </xsl:stylesheet>
      </root>)";

  FrameTestHelpers::LoadFrame(MainFrame(), kURL);

  Document* transformed_document = MainFrame()->GetFrame()->GetDocument();
  Document* source_document =
      DocumentXSLT::From(*transformed_document).TransformSourceDocument();
  // Intentionally use strict equality here rather than CanAccess(), to check
  // that the origin is aliased from the original document.
  EXPECT_EQ(source_document->GetSecurityOrigin(),
            transformed_document->GetSecurityOrigin());
  EXPECT_EQ(source_document->CookieURL(), transformed_document->CookieURL());
  EXPECT_TRUE(transformed_document->GetContentSecurityPolicy()->Subsumes(
      *source_document->GetContentSecurityPolicy()));
  EXPECT_TRUE(source_document->GetContentSecurityPolicy()->Subsumes(
      *transformed_document->GetContentSecurityPolicy()));
}

}  // namespace blink
