// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/url/DOMURLUtilsReadOnly.h"

#include <gtest/gtest.h>
#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/Document.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/bindings/ScriptState.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

// Override the two unimplemented methods so we can test the class's
// functionality.
class DOMURLUtilsReadOnlyTestImpl : public DOMURLUtilsReadOnly {
 public:
  DOMURLUtilsReadOnlyTestImpl(const KURL& url) : url_(url) {}
  KURL Url() const override { return url_; }
  String Input() const override { return ""; }

 private:
  KURL url_;
};

class DOMURLUtilsReadOnlyTest : public ::testing::Test {
 public:
  DOMURLUtilsReadOnlyTest() : page_(DummyPageHolder::Create(IntSize(1, 1))) {}

  void SetSecurityOrigin(
      const String& origin,
      DOMURLUtilsReadOnly::FileOriginSerialization serialize_as) {
    KURL page_url(origin);
    scoped_refptr<SecurityOrigin> page_origin =
        SecurityOrigin::Create(page_url);
    if (serialize_as == DOMURLUtilsReadOnly::FileOriginSerialization::kFile &&
        page_origin->IsLocal()) {
      page_origin->GrantLocalAccessFromLocalOrigin();
    }
    page_->GetDocument().SetSecurityOrigin(page_origin);
  }

  ScriptState* GetScriptState() {
    return ToScriptStateForMainWorld(page_->GetDocument().GetFrame());
  }

 private:
  std::unique_ptr<DummyPageHolder> page_;
};

TEST_F(DOMURLUtilsReadOnlyTest, StaticFileOrigin) {
  KURL file("file:///this/is/a/file/url");

  EXPECT_EQ("null", DOMURLUtilsReadOnly::origin(file));
  EXPECT_EQ("null",
            DOMURLUtilsReadOnly::origin(
                file, DOMURLUtilsReadOnly::FileOriginSerialization::kNull));
  EXPECT_EQ("file://",
            DOMURLUtilsReadOnly::origin(
                file, DOMURLUtilsReadOnly::FileOriginSerialization::kFile));
}

TEST_F(DOMURLUtilsReadOnlyTest, FileOrigin) {
  KURL file("file:///this/is/a/file/url");
  DOMURLUtilsReadOnlyTestImpl url_utils(file);

  // `file:` is "null" by default.
  EXPECT_EQ("null", url_utils.origin(GetScriptState()));

  // `file:` is always "null" on webby pages.
  SetSecurityOrigin("http://example.test/",
                    DOMURLUtilsReadOnly::FileOriginSerialization::kNull);
  EXPECT_EQ("null", url_utils.origin(GetScriptState()));
  SetSecurityOrigin("http://example.test/",
                    DOMURLUtilsReadOnly::FileOriginSerialization::kFile);
  EXPECT_EQ("null", url_utils.origin(GetScriptState()));

  // `file:` is either "null" or "file:" on `file:` pages
  SetSecurityOrigin("file:///yay/",
                    DOMURLUtilsReadOnly::FileOriginSerialization::kNull);
  EXPECT_EQ("null", url_utils.origin(GetScriptState()));
  SetSecurityOrigin("file:///yay/",
                    DOMURLUtilsReadOnly::FileOriginSerialization::kFile);
  EXPECT_EQ("file://", url_utils.origin(GetScriptState()));
}

}  // namespace blink
