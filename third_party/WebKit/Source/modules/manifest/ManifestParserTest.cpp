// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/manifest/ManifestParser.h"

#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/manifest/manifest.mojom-blink.h"

namespace blink {
namespace {

uint32_t ExtractColor(int64_t color) {
  return reinterpret_cast<uint32_t&>(color);
}

class ManifestParserTest : public testing::Test {
 protected:
  ManifestParserTest()
      : default_document_url_(KURL(), "http://foo.com/index.html"),
        default_manifest_url_(KURL(), "http://foo.com/manifest.json") {}
  ~ManifestParserTest() override {}

  mojom::blink::ManifestPtr ParseManifestWithURLs(const String& data,
                                                  const KURL& manifest_url,
                                                  const KURL& document_url) {
    ManifestParser parser(data, manifest_url, document_url);
    parser.Parse();
    errors_.clear();
    for (auto& error : parser.TakeErrors()) {
      errors_.push_back(error->message);
    }
    return parser.Manifest();
  }

  mojom::blink::ManifestPtr ParseManifest(const String& data) {
    return ParseManifestWithURLs(data, default_manifest_url_,
                                 default_document_url_);
  }

  const Vector<String>& errors() const { return errors_; }

  unsigned int GetErrorCount() const { return errors_.size(); }

  const KURL default_document_url_;
  const KURL default_manifest_url_;

 private:
  Vector<String> errors_;

  DISALLOW_COPY_AND_ASSIGN(ManifestParserTest);
};

TEST_F(ManifestParserTest, EmptyStringNull) {
  auto manifest = ParseManifest("");

  // This Manifest is not a valid JSON object, it's a parsing error.
  ASSERT_EQ(1u, GetErrorCount());
  EXPECT_EQ("Invalid JSON.", errors()[0]);

  // A parsing error is equivalent to an empty manifest.
  EXPECT_FALSE(manifest);
}

TEST_F(ManifestParserTest, ValidNoContentParses) {
  auto manifest = ParseManifest("{}");

  // Empty Manifest is not a parsing error.
  EXPECT_EQ(0u, GetErrorCount());

  // Check that all the fields are null in that case.
  EXPECT_FALSE(manifest);
}

TEST_F(ManifestParserTest, MultipleErrorsReporting) {
  auto manifest = ParseManifest(
      "{ \"name\": 42, \"short_name\": 4,"
      "\"orientation\": {}, \"display\": \"foo\","
      "\"start_url\": null, \"icons\": {}, \"theme_color\": 42,"
      "\"background_color\": 42 }");

  ASSERT_EQ(8u, GetErrorCount());

  EXPECT_EQ("property 'name' ignored, type string expected.", errors()[0]);
  EXPECT_EQ("property 'short_name' ignored, type string expected.",
            errors()[1]);
  EXPECT_EQ("property 'start_url' ignored, type string expected.", errors()[2]);
  EXPECT_EQ("unknown 'display' value ignored.", errors()[3]);
  EXPECT_EQ("property 'orientation' ignored, type string expected.",
            errors()[4]);
  EXPECT_EQ("property 'icons' ignored, type array expected.", errors()[5]);
  EXPECT_EQ("property 'theme_color' ignored, type string expected.",
            errors()[6]);
  EXPECT_EQ("property 'background_color' ignored, type string expected.",
            errors()[7]);
}

TEST_F(ManifestParserTest, NameParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"name\": \"foo\" }");
    EXPECT_EQ(manifest->name, "foo");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest("{ \"name\": \"  foo  \" }");
    EXPECT_EQ(manifest->name, "foo");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"name\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'name' ignored, type string expected.", errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"name\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'name' ignored, type string expected.", errors()[0]);
  }
}

TEST_F(ManifestParserTest, ShortNameParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"short_name\": \"foo\" }");
    EXPECT_EQ(manifest->short_name, "foo");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest("{ \"short_name\": \"  foo  \" }");
    EXPECT_EQ(manifest->short_name, "foo");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"short_name\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'short_name' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"short_name\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'short_name' ignored, type string expected.",
              errors()[0]);
  }
}

TEST_F(ManifestParserTest, StartURLParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"start_url\": \"land.html\" }");
    ASSERT_EQ(manifest->start_url, KURL(default_document_url_, "land.html"));
    ASSERT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    auto manifest = ParseManifest("{ \"start_url\": \"  land.html  \" }");
    ASSERT_EQ(manifest->start_url, KURL(default_document_url_, "land.html"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"start_url\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'start_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"start_url\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'start_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a valid URL.
  {
    auto manifest =
        ParseManifest("{ \"start_url\": \"http://www.google.ca:a\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'start_url' ignored, URL is invalid.", errors()[0]);
  }

  // Absolute start_url, same origin with document.
  {
    auto manifest =
        ParseManifestWithURLs("{ \"start_url\": \"http://foo.com/land.html\" }",
                              KURL(KURL(), "http://foo.com/manifest.json"),
                              KURL(KURL(), "http://foo.com/index.html"));
    ASSERT_EQ(manifest->start_url, KURL(KURL(), "http://foo.com/land.html"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Absolute start_url, cross origin with document.
  {
    auto manifest =
        ParseManifestWithURLs("{ \"start_url\": \"http://bar.com/land.html\" }",
                              KURL(KURL(), "http://foo.com/manifest.json"),
                              KURL(KURL(), "http://foo.com/index.html"));
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'start_url' ignored, should "
        "be same origin as document.",
        errors()[0]);
  }

  // Resolving has to happen based on the manifest_url.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"start_url\": \"land.html\" }",
        KURL(KURL(), "http://foo.com/landing/manifest.json"),
        KURL(KURL(), "http://foo.com/index.html"));
    ASSERT_EQ(manifest->start_url,
              KURL(KURL(), "http://foo.com/landing/land.html"));
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, ScopeParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest(
        "{ \"scope\": \"land\", \"start_url\": \"land/landing.html\" }");
    ASSERT_EQ(manifest->scope, KURL(default_document_url_, "land"));
    ASSERT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    auto manifest = ParseManifest(
        "{ \"scope\": \"  land  \", \"start_url\": \"land/landing.html\" }");
    ASSERT_EQ(manifest->scope, KURL(default_document_url_, "land"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"scope\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'scope' ignored, type string expected.", errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"scope\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'scope' ignored, type string expected.", errors()[0]);
  }

  // Absolute scope, start URL is in scope.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://foo.com/land/landing.html\" }",
        KURL(KURL(), "http://foo.com/manifest.json"),
        KURL(KURL(), "http://foo.com/index.html"));
    ASSERT_EQ(manifest->scope, KURL(KURL(), "http://foo.com/land"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Absolute scope, start URL is not in scope.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://foo.com/index.html\" }",
        KURL(KURL(), "http://foo.com/manifest.json"),
        KURL(KURL(), "http://foo.com/index.html"));
    ASSERT_TRUE(manifest);
    EXPECT_TRUE(manifest->scope.IsNull());
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'scope' ignored. Start url should be within scope "
        "of scope URL.",
        errors()[0]);
  }

  // Absolute scope, start URL has different origin than scope URL.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://bar.com/land/landing.html\" }",
        KURL(KURL(), "http://foo.com/manifest.json"),
        KURL(KURL(), "http://foo.com/index.html"));
    EXPECT_FALSE(manifest);
    ASSERT_EQ(2u, GetErrorCount());
    EXPECT_EQ(
        "property 'start_url' ignored, should be same origin as document.",
        errors()[0]);
    EXPECT_EQ(
        "property 'scope' ignored. Start url should be within scope "
        "of scope URL.",
        errors()[1]);
  }

  // scope and start URL have diferent origin than document URL.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"scope\": \"http://foo.com/land\", "
        "\"start_url\": \"http://foo.com/land/landing.html\" }",
        KURL(KURL(), "http://foo.com/manifest.json"),
        KURL(KURL(), "http://bar.com/index.html"));
    EXPECT_FALSE(manifest);
    ASSERT_EQ(2u, GetErrorCount());
    EXPECT_EQ(
        "property 'start_url' ignored, should be same origin as document.",
        errors()[0]);
    EXPECT_EQ("property 'scope' ignored, should be same origin as document.",
              errors()[1]);
  }

  // No start URL. Document URL is in scope.
  {
    auto manifest =
        ParseManifestWithURLs("{ \"scope\": \"http://foo.com/land\" }",
                              KURL(KURL(), "http://foo.com/manifest.json"),
                              KURL(KURL(), "http://foo.com/land/index.html"));
    ASSERT_EQ(manifest->scope, KURL(KURL(), "http://foo.com/land"));
    ASSERT_EQ(0u, GetErrorCount());
  }

  // No start URL. Document is out of scope.
  {
    auto manifest =
        ParseManifestWithURLs("{ \"scope\": \"http://foo.com/land\" }",
                              KURL(KURL(), "http://foo.com/manifest.json"),
                              KURL(KURL(), "http://foo.com/index.html"));
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'scope' ignored. Start url should be within scope "
        "of scope URL.",
        errors()[0]);
  }

  // Resolving has to happen based on the manifest_url.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"scope\": \"treasure\" }",
        KURL(KURL(), "http://foo.com/map/manifest.json"),
        KURL(KURL(), "http://foo.com/map/treasure/island/index.html"));
    ASSERT_EQ(manifest->scope, KURL(KURL(), "http://foo.com/map/treasure"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Scope is parent directory.
  {
    auto manifest =
        ParseManifestWithURLs("{ \"scope\": \"..\" }",
                              KURL(KURL(), "http://foo.com/map/manifest.json"),
                              KURL(KURL(), "http://foo.com/index.html"));
    ASSERT_EQ(manifest->scope, KURL(KURL(), "http://foo.com/"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Scope tries to go up past domain.
  {
    auto manifest =
        ParseManifestWithURLs("{ \"scope\": \"../..\" }",
                              KURL(KURL(), "http://foo.com/map/manifest.json"),
                              KURL(KURL(), "http://foo.com/index.html"));
    ASSERT_EQ(manifest->scope, KURL(KURL(), "http://foo.com/"));
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, DisplayParserRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"display\": \"browser\" }");
    EXPECT_EQ(manifest->display, blink::kWebDisplayModeBrowser);
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest("{ \"display\": \"  browser  \" }");
    EXPECT_EQ(manifest->display, blink::kWebDisplayModeBrowser);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"display\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'display' ignored,"
        " type string expected.",
        errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"display\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'display' ignored,"
        " type string expected.",
        errors()[0]);
  }

  // Parse fails if string isn't known.
  {
    auto manifest = ParseManifest("{ \"display\": \"browser_something\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("unknown 'display' value ignored.", errors()[0]);
  }

  // Accept 'fullscreen'.
  {
    auto manifest = ParseManifest("{ \"display\": \"fullscreen\" }");
    EXPECT_EQ(manifest->display, blink::kWebDisplayModeFullscreen);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'fullscreen'.
  {
    auto manifest = ParseManifest("{ \"display\": \"standalone\" }");
    EXPECT_EQ(manifest->display, blink::kWebDisplayModeStandalone);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'minimal-ui'.
  {
    auto manifest = ParseManifest("{ \"display\": \"minimal-ui\" }");
    EXPECT_EQ(manifest->display, blink::kWebDisplayModeMinimalUi);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'browser'.
  {
    auto manifest = ParseManifest("{ \"display\": \"browser\" }");
    EXPECT_EQ(manifest->display, blink::kWebDisplayModeBrowser);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Case insensitive.
  {
    auto manifest = ParseManifest("{ \"display\": \"BROWSER\" }");
    EXPECT_EQ(manifest->display, blink::kWebDisplayModeBrowser);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, OrientationParserRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"natural\" }");
    EXPECT_EQ(manifest->orientation, blink::kWebScreenOrientationLockNatural);
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"natural\" }");
    EXPECT_EQ(manifest->orientation, blink::kWebScreenOrientationLockNatural);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"orientation\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'orientation' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if name isn't a string.
  {
    auto manifest = ParseManifest("{ \"orientation\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'orientation' ignored, type string expected.",
              errors()[0]);
  }

  // Parse fails if string isn't known.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"naturalish\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("unknown 'orientation' value ignored.", errors()[0]);
  }

  // Accept 'any'.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"any\" }");
    EXPECT_EQ(manifest->orientation, blink::kWebScreenOrientationLockAny);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'natural'.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"natural\" }");
    EXPECT_EQ(manifest->orientation, blink::kWebScreenOrientationLockNatural);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'landscape'.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"landscape\" }");
    EXPECT_EQ(manifest->orientation, blink::kWebScreenOrientationLockLandscape);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'landscape-primary'.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"landscape-primary\" }");
    EXPECT_EQ(manifest->orientation,
              blink::kWebScreenOrientationLockLandscapePrimary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'landscape-secondary'.
  {
    auto manifest =
        ParseManifest("{ \"orientation\": \"landscape-secondary\" }");
    EXPECT_EQ(manifest->orientation,
              blink::kWebScreenOrientationLockLandscapeSecondary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'portrait'.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"portrait\" }");
    EXPECT_EQ(manifest->orientation, blink::kWebScreenOrientationLockPortrait);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'portrait-primary'.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"portrait-primary\" }");
    EXPECT_EQ(manifest->orientation,
              blink::kWebScreenOrientationLockPortraitPrimary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept 'portrait-secondary'.
  {
    auto manifest =
        ParseManifest("{ \"orientation\": \"portrait-secondary\" }");
    EXPECT_EQ(manifest->orientation,
              blink::kWebScreenOrientationLockPortraitSecondary);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Case insensitive.
  {
    auto manifest = ParseManifest("{ \"orientation\": \"LANDSCAPE\" }");
    EXPECT_EQ(manifest->orientation, blink::kWebScreenOrientationLockLandscape);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, IconsParseRules) {
  // Smoke test: if no icon, empty list.
  {
    auto manifest = ParseManifest("{ \"icons\": [] }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: if empty icon, empty list.
  {
    auto manifest = ParseManifest("{ \"icons\": [ {} ] }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: icon with invalid src, empty list.
  {
    auto manifest = ParseManifest("{ \"icons\": [ { \"icons\": [] } ] }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: if icon with empty src, it will be present in the list.
  {
    auto manifest = ParseManifest("{ \"icons\": [ { \"src\": \"\" } ] }");
    EXPECT_EQ(manifest->icons.size(), 1u);
    EXPECT_EQ(manifest->icons[0]->src,
              KURL(KURL(), "http://foo.com/manifest.json"));
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: if one icons with valid src, it will be present in the list.
  {
    auto manifest = ParseManifest("{ \"icons\": [{ \"src\": \"foo.jpg\" }] }");
    EXPECT_EQ(manifest->icons.size(), 1u);
    EXPECT_EQ(manifest->icons[0]->src, KURL(KURL(), "http://foo.com/foo.jpg"));
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, IconSrcParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"icons\": [ {\"src\": \"foo.png\" } ] }");
    EXPECT_EQ(manifest->icons[0]->src, KURL(default_document_url_, "foo.png"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    auto manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"   foo.png   \" } ] }");
    EXPECT_EQ(manifest->icons[0]->src, KURL(default_document_url_, "foo.png"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"icons\": [ {\"src\": {} } ] }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'src' ignored, type string expected.", errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"icons\": [ {\"src\": 42 } ] }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'src' ignored, type string expected.", errors()[0]);
  }

  // Resolving has to happen based on the document_url.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"icons\": [ {\"src\": \"icons/foo.png\" } ] }",
        KURL(KURL(), "http://foo.com/landing/index.html"),
        default_manifest_url_);
    EXPECT_EQ(manifest->icons[0]->src,
              KURL(KURL(), "http://foo.com/landing/icons/foo.png"));
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, IconTypeParseRules) {
  // Smoke test.
  {
    auto manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"\", \"type\": \"foo\" } ] }");
    EXPECT_EQ(manifest->icons[0]->type, "foo");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        " \"type\": \"  foo  \" } ] }");
    EXPECT_EQ(manifest->icons[0]->type, "foo");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    auto manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"\", \"type\": {} } ] }");
    EXPECT_TRUE(manifest->icons[0]->type.IsEmpty());
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'type' ignored, type string expected.", errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    auto manifest =
        ParseManifest("{ \"icons\": [ {\"src\": \"\", \"type\": 42 } ] }");
    EXPECT_TRUE(manifest->icons[0]->type.IsEmpty());
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'type' ignored, type string expected.", errors()[0]);
  }
}

TEST_F(ManifestParserTest, IconSizesParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42x42\" } ] }");
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"  42x42  \" } ] }");
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Ignore sizes if property isn't a string.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": {} } ] }");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 0u);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'sizes' ignored, type string expected.", errors()[0]);
  }

  // Ignore sizes if property isn't a string.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": 42 } ] }");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 0u);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'sizes' ignored, type string expected.", errors()[0]);
  }

  // Smoke test: value correctly parsed.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42x42  48x48\" } ] }");
    EXPECT_EQ(manifest->icons[0]->sizes[0], WebSize(42, 42));
    EXPECT_EQ(manifest->icons[0]->sizes[1], WebSize(48, 48));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // <WIDTH>'x'<HEIGHT> and <WIDTH>'X'<HEIGHT> are equivalent.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42X42  48X48\" } ] }");
    EXPECT_EQ(manifest->icons[0]->sizes[0], WebSize(42, 42));
    EXPECT_EQ(manifest->icons[0]->sizes[1], WebSize(48, 48));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Twice the same value is parsed twice.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"42X42  42x42\" } ] }");
    EXPECT_EQ(manifest->icons[0]->sizes[0], WebSize(42, 42));
    EXPECT_EQ(manifest->icons[0]->sizes[1], WebSize(42, 42));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Width or height can't start with 0.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"004X007  042x00\" } ] }");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 0u);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("found icon with no valid size.", errors()[0]);
  }

  // Width and height MUST contain digits.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"e4X1.0  55ax1e10\" } ] }");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 0u);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("found icon with no valid size.", errors()[0]);
  }

  // 'any' is correctly parsed and transformed to WebSize(0,0).
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"any AnY ANY aNy\" } ] }");
    WebSize any = WebSize(0, 0);
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 4u);
    EXPECT_EQ(manifest->icons[0]->sizes[0], any);
    EXPECT_EQ(manifest->icons[0]->sizes[1], any);
    EXPECT_EQ(manifest->icons[0]->sizes[2], any);
    EXPECT_EQ(manifest->icons[0]->sizes[3], any);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Some invalid width/height combinations.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"sizes\": \"x 40xx 1x2x3 x42 42xx42\" } ] }");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->icons[0]->sizes.size(), 0u);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("found icon with no valid size.", errors()[0]);
  }
}

TEST_F(ManifestParserTest, IconPurposeParseRules) {
  const String kPurposeParseStringError =
      "property 'purpose' ignored, type string expected.";
  const String kPurposeInvalidValueError =
      "found icon with invalid purpose. "
      "Using default value 'any'.";

  // Smoke test.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"any\" } ] }");
    EXPECT_EQ(manifest->icons[0]->purpose.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim leading and trailing whitespaces.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"  any  \" } ] }");
    EXPECT_EQ(manifest->icons[0]->purpose.size(), 1u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // 'any' is added when property isn't present.
  {
    auto manifest = ParseManifest("{ \"icons\": [ {\"src\": \"\" } ] }");
    EXPECT_EQ(manifest->icons[0]->purpose.size(), 1u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::ANY);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // 'any' is added with error message when property isn't a string (is a
  // number).
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": 42 } ] }");
    EXPECT_EQ(manifest->icons[0]->purpose.size(), 1u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::ANY);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeParseStringError, errors()[0]);
  }

  // 'any' is added with error message when property isn't a string (is a
  // dictionary).
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": {} } ] }");
    EXPECT_EQ(manifest->icons[0]->purpose.size(), 1u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::ANY);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeParseStringError, errors()[0]);
  }

  // Smoke test: values correctly parsed.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"Any Badge\" } ] }");
    ASSERT_EQ(manifest->icons[0]->purpose.size(), 2u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::ANY);
    EXPECT_EQ(manifest->icons[0]->purpose[1],
              mojom::blink::ManifestIcon::Purpose::BADGE);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces between values.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"  Any   Badge  \" } ] }");
    ASSERT_EQ(manifest->icons[0]->purpose.size(), 2u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::ANY);
    EXPECT_EQ(manifest->icons[0]->purpose[1],
              mojom::blink::ManifestIcon::Purpose::BADGE);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Twice the same value is parsed twice.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"badge badge\" } ] }");
    ASSERT_EQ(manifest->icons[0]->purpose.size(), 2u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::BADGE);
    EXPECT_EQ(manifest->icons[0]->purpose[1],
              mojom::blink::ManifestIcon::Purpose::BADGE);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Invalid icon purpose is ignored.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"badge notification\" } ] }");
    ASSERT_EQ(manifest->icons[0]->purpose.size(), 1u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::BADGE);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeInvalidValueError, errors()[0]);
  }

  // 'any' is added when developer-supplied purpose is invalid.
  {
    auto manifest = ParseManifest(
        "{ \"icons\": [ {\"src\": \"\","
        "\"purpose\": \"notification\" } ] }");
    ASSERT_EQ(manifest->icons[0]->purpose.size(), 1u);
    EXPECT_EQ(manifest->icons[0]->purpose[0],
              mojom::blink::ManifestIcon::Purpose::ANY);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(kPurposeInvalidValueError, errors()[0]);
  }
}

TEST_F(ManifestParserTest, ShareTargetParseRules) {
  // Contains share_target field but no keys.
  {
    auto manifest = ParseManifest("{ \"share_target\": {} }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Key in share_target that isn't valid.
  {
    auto manifest = ParseManifest(
        "{ \"share_target\": {\"incorrect_key\": \"some_value\" } }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, ShareTargetUrlTemplateParseRules) {
  // Contains share_target and url_template, but url_template is empty.
  {
    auto manifest =
        ParseManifest("{ \"share_target\": { \"url_template\": \"\" } }");
    ASSERT_TRUE(manifest->share_target);
    EXPECT_EQ(manifest->share_target->url_template, "");
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    auto manifest =
        ParseManifest("{ \"share_target\": { \"url_template\": {} } }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'url_template' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    auto manifest =
        ParseManifest("{ \"share_target\": { \"url_template\": 42 } }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'url_template' ignored, type string expected.",
              errors()[0]);
  }

  // Smoke test: Contains share_target and url_template, and url_template is
  // valid template.
  {
    auto manifest = ParseManifest(
        "{ \"share_target\": {\"url_template\": \"share/?title={title}\" } }");
    ASSERT_TRUE(manifest->share_target);
    EXPECT_EQ(manifest->share_target->url_template, "share/?title={title}");
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Smoke test: Contains share_target and url_template, and url_template is
  // invalid template.
  {
    auto manifest = ParseManifest(
        "{ \"share_target\": {\"url_template\": \"share/?title={title\" } }");
    ASSERT_TRUE(manifest->share_target);
    EXPECT_EQ(manifest->share_target->url_template, "share/?title={title");
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, RelatedApplicationsParseRules) {
  // If no application, empty list.
  {
    auto manifest = ParseManifest("{ \"related_applications\": []}");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // If empty application, empty list.
  {
    auto manifest = ParseManifest("{ \"related_applications\": [{}]}");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("'platform' is a required field, related application ignored.",
              errors()[0]);
  }

  // If invalid platform, application is ignored.
  {
    auto manifest =
        ParseManifest("{ \"related_applications\": [{\"platform\": 123}]}");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(2u, GetErrorCount());
    EXPECT_EQ("property 'platform' ignored, type string expected.",
              errors()[0]);
    EXPECT_EQ(
        "'platform' is a required field, "
        "related application ignored.",
        errors()[1]);
  }

  // If missing platform, application is ignored.
  {
    auto manifest =
        ParseManifest("{ \"related_applications\": [{\"id\": \"foo\"}]}");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("'platform' is a required field, related application ignored.",
              errors()[0]);
  }

  // If missing id and url, application is ignored.
  {
    auto manifest = ParseManifest(
        "{ \"related_applications\": [{\"platform\": \"play\"}]}");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("one of 'url' or 'id' is required, related application ignored.",
              errors()[0]);
  }

  // Valid application, with url.
  {
    auto manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"play\", \"url\": \"http://www.foo.com\"}]}");
    EXPECT_EQ(0u, GetErrorCount());
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->related_applications.size(), 1u);
    EXPECT_EQ(manifest->related_applications[0]->platform, "play");
    EXPECT_EQ(manifest->related_applications[0]->url,
              KURL(KURL(), "http://www.foo.com/"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Application with an invalid url.
  {
    auto manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"play\", \"url\": \"http://www.foo.com:co&uk\"}]}");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(2u, GetErrorCount());
    EXPECT_EQ("property 'url' ignored, URL is invalid.", errors()[0]);
    EXPECT_EQ("one of 'url' or 'id' is required, related application ignored.",
              errors()[1]);
  }

  // Valid application, with id.
  {
    auto manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"itunes\", \"id\": \"foo\"}]}");
    EXPECT_EQ(manifest->related_applications.size(), 1u);
    EXPECT_EQ(manifest->related_applications[0]->platform, "itunes");
    EXPECT_EQ(manifest->related_applications[0]->id, "foo");
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // All valid applications are in list.
  {
    auto manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"play\", \"id\": \"foo\"},"
        "{\"platform\": \"itunes\", \"id\": \"bar\"}]}");
    EXPECT_EQ(manifest->related_applications.size(), 2u);
    EXPECT_EQ(manifest->related_applications[0]->platform, "play");
    EXPECT_EQ(manifest->related_applications[0]->id, "foo");
    EXPECT_EQ(manifest->related_applications[1]->platform, "itunes");
    EXPECT_EQ(manifest->related_applications[1]->id, "bar");
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Two invalid applications and one valid. Only the valid application should
  // be in the list.
  {
    auto manifest = ParseManifest(
        "{ \"related_applications\": ["
        "{\"platform\": \"itunes\"},"
        "{\"platform\": \"play\", \"id\": \"foo\"},"
        "{}]}");
    EXPECT_EQ(manifest->related_applications.size(), 1u);
    EXPECT_EQ(manifest->related_applications[0]->platform, "play");
    EXPECT_EQ(manifest->related_applications[0]->id, "foo");
    EXPECT_TRUE(manifest);
    ASSERT_EQ(2u, GetErrorCount());
    EXPECT_EQ("one of 'url' or 'id' is required, related application ignored.",
              errors()[0]);
    EXPECT_EQ("'platform' is a required field, related application ignored.",
              errors()[1]);
  }
}

TEST_F(ManifestParserTest, ParsePreferRelatedApplicationsParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"prefer_related_applications\": true }");
    EXPECT_TRUE(manifest->prefer_related_applications);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if the property isn't a boolean.
  {
    auto manifest = ParseManifest("{ \"prefer_related_applications\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'prefer_related_applications' "
        "ignored, type boolean expected.",
        errors()[0]);
  }
  {
    auto manifest =
        ParseManifest("{ \"prefer_related_applications\": \"true\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'prefer_related_applications' "
        "ignored, type boolean expected.",
        errors()[0]);
  }
  {
    auto manifest = ParseManifest("{ \"prefer_related_applications\": 1 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'prefer_related_applications' "
        "ignored, type boolean expected.",
        errors()[0]);
  }

  // "False" should set the boolean false without throwing errors.
  {
    auto manifest = ParseManifest("{ \"prefer_related_applications\": false }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, ThemeColorParserRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"#FF0000\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0xFFFF0000u);
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"  blue   \" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if theme_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"theme_color\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"theme_color\": false }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"theme_color\": null }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"theme_color\": [] }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if theme_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"theme_color\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, type string expected.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"foo(bar)\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'theme_color' ignored,"
        " 'foo(bar)' is not a valid color.",
        errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"bleu\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'theme_color' ignored, 'bleu' is not a valid color.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"FF00FF\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'theme_color' ignored, 'FF00FF'"
        " is not a valid color.",
        errors()[0]);
  }

  // Parse fails if multiple values for theme_color are given.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"#ABC #DEF\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'theme_color' ignored, "
        "'#ABC #DEF' is not a valid color.",
        errors()[0]);
  }

  // Parse fails if multiple values for theme_color are given.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"#AABBCC #DDEEFF\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'theme_color' ignored, "
        "'#AABBCC #DDEEFF' is not a valid color.",
        errors()[0]);
  }

  // Accept CSS color keyword format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"blue\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS color keyword format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"chartreuse\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0xFF7FFF00u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"#FFF\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0xFFFFFFFFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"#ABC\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0xFFAABBCCu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RRGGBB format.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"#FF0000\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0xFFFF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept translucent colors.
  {
    auto manifest = ParseManifest(
        "{ \"theme_color\": \"rgba(255,0,0,"
        "0.4)\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0x66FF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept transparent colors.
  {
    auto manifest = ParseManifest("{ \"theme_color\": \"rgba(0,0,0,0)\" }");
    EXPECT_EQ(ExtractColor(manifest->theme_color), 0x00000000u);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, BackgroundColorParserRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"#FF0000\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0xFFFF0000u);
    EXPECT_TRUE(manifest);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"  blue   \" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if background_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"background_color\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"background_color\": false }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"background_color\": null }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"background_color\": [] }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if background_color isn't a string.
  {
    auto manifest = ParseManifest("{ \"background_color\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'background_color' ignored, type string expected.",
              errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"foo(bar)\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'background_color' ignored,"
        " 'foo(bar)' is not a valid color.",
        errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"bleu\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'background_color' ignored,"
        " 'bleu' is not a valid color.",
        errors()[0]);
  }

  // Parse fails if string is not in a known format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"FF00FF\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'background_color' ignored,"
        " 'FF00FF' is not a valid color.",
        errors()[0]);
  }

  // Parse fails if multiple values for background_color are given.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"#ABC #DEF\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'background_color' ignored, "
        "'#ABC #DEF' is not a valid color.",
        errors()[0]);
  }

  // Parse fails if multiple values for background_color are given.
  {
    auto manifest =
        ParseManifest("{ \"background_color\": \"#AABBCC #DDEEFF\" }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'background_color' ignored, "
        "'#AABBCC #DDEEFF' is not a valid color.",
        errors()[0]);
  }

  // Accept CSS color keyword format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"blue\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0xFF0000FFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS color keyword format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"chartreuse\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0xFF7FFF00u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"#FFF\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0xFFFFFFFFu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RGB format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"#ABC\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0xFFAABBCCu);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept CSS RRGGBB format.
  {
    auto manifest = ParseManifest("{ \"background_color\": \"#FF0000\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0xFFFF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept translucent colors.
  {
    auto manifest = ParseManifest(
        "{ \"background_color\": \"rgba(255,0,0,"
        "0.4)\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0x66FF0000u);
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Accept transparent colors.
  {
    auto manifest = ParseManifest(
        "{ \"background_color\": \"rgba(0,0,0,"
        "0)\" }");
    EXPECT_EQ(ExtractColor(manifest->background_color), 0x00000000u);
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, SplashScreenUrlParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"splash_screen_url\": \"splash.html\" }");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->splash_screen_url,
              KURL(default_document_url_, "splash.html"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Whitespaces.
  {
    auto manifest =
        ParseManifest("{ \"splash_screen_url\": \"    splash.html\" }");
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->splash_screen_url,
              KURL(default_document_url_, "splash.html"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"splash_screen_url\": {} }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'splash_screen_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a string.
  {
    auto manifest = ParseManifest("{ \"splash_screen_url\": 42 }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'splash_screen_url' ignored, type string expected.",
              errors()[0]);
  }

  // Don't parse if property isn't a valid URL.
  {
    auto manifest =
        ParseManifest("{ \"splash_screen_url\": \"http://www.google.ca:a\" }");
    EXPECT_FALSE(manifest);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'splash_screen_url' ignored, URL is invalid.",
              errors()[0]);
  }

  // Absolute splash_screen_url, same origin with document.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"splash_screen_url\": \"http://foo.com/splash.html\" }",
        KURL("http://foo.com/manifest.json"),
        KURL("http://foo.com/index.html"));
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->splash_screen_url,
              KURL(KURL(), "http://foo.com/splash.html"));
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Absolute splash_screen_url, cross origin with document.
  {
    auto manifest = ParseManifestWithURLs(
        "{ \"splash_screen_url\": \"http://bar.com/splash.html\" }",
        KURL("http://foo.com/manifest.json"),
        KURL("http://foo.com/index.html"));
    EXPECT_FALSE(manifest);
    EXPECT_EQ(1u, GetErrorCount());
    EXPECT_EQ(
        "property 'splash_screen_url' ignored, should "
        "be same origin as document.",
        errors()[0]);
  }

  // Resolving has to happen based on the manifest_url.
  {
    auto manifest =
        ParseManifestWithURLs("{ \"splash_screen_url\": \"splash.html\" }",
                              KURL("http://foo.com/splashy/manifest.json"),
                              KURL("http://foo.com/index.html"));
    ASSERT_TRUE(manifest);
    EXPECT_EQ(manifest->splash_screen_url,
              KURL(KURL(), "http://foo.com/splashy/splash.html"));
    EXPECT_EQ(0u, GetErrorCount());
  }
}

TEST_F(ManifestParserTest, GCMSenderIDParseRules) {
  // Smoke test.
  {
    auto manifest = ParseManifest("{ \"gcm_sender_id\": \"foo\" }");
    EXPECT_EQ(manifest->gcm_sender_id, "foo");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Trim whitespaces.
  {
    auto manifest = ParseManifest("{ \"gcm_sender_id\": \"  foo  \" }");
    EXPECT_EQ(manifest->gcm_sender_id, "foo");
    EXPECT_EQ(0u, GetErrorCount());
  }

  // Don't parse if the property isn't a string.
  {
    auto manifest = ParseManifest("{ \"gcm_sender_id\": {} }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'gcm_sender_id' ignored, type string expected.",
              errors()[0]);
  }
  {
    auto manifest = ParseManifest("{ \"gcm_sender_id\": 42 }");
    EXPECT_FALSE(manifest);
    ASSERT_EQ(1u, GetErrorCount());
    EXPECT_EQ("property 'gcm_sender_id' ignored, type string expected.",
              errors()[0]);
  }
}

}  // namespace
}  // namespace blink
