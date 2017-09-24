// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class OriginManifestParserTest : public testing::Test {};

// Note that we do not initialize the persistent store here since that
// is subject to testing the persistent store itself.

TEST_F(OriginManifestParserTest, ParseEmptyOriginManifest) {
  std::string origin = "https://a.com/";
  std::string version = "version1";
  std::string json = "";
  EXPECT_TRUE(
      OriginManifestParser::Parse(origin, version, json).Equals(nullptr));

  json = "{ }";
  mojom::OriginManifestPtr om =
      OriginManifestParser::Parse(origin, version, json);
  ASSERT_FALSE(om.Equals(nullptr));
  EXPECT_TRUE(om->origin == origin);
  EXPECT_TRUE(om->version == version);
}

TEST_F(OriginManifestParserTest, ParseOriginManifestWithCSPs) {
  std::string origin = "https://a.com/";
  std::string version = "version1";
  std::string policy0 = "";
  std::string policy1 = "default-src 'none';";
  std::string policy2 = "default-src 'none';";
  std::string json =
      "{"
      "\"content-security-policy\": ["
      "{ },"
      "{"
      "\"policy\": \"" +
      policy1 +
      "\","
      "\"disposition\": \"report-only\","
      "\"allow-override\": true"
      "},"
      "{"
      "\"policy\": \"" +
      policy2 +
      "\","
      "\"disposition\": \"enforce\","
      "\"allow-override\": false"
      "},"
      "],"
      "}";

  mojom::OriginManifestPtr om =
      OriginManifestParser::Parse(origin, version, json);
  ASSERT_FALSE(om.Equals(nullptr));
  ASSERT_EQ(om->csps.size(), 3ul);

  EXPECT_TRUE(om->csps[0]->policy == policy0);
  EXPECT_TRUE(om->csps[0]->disposition == mojom::Disposition::ENFORCE);
  EXPECT_FALSE(om->csps[0]->allowOverride);

  EXPECT_TRUE(om->csps[1]->policy == policy1);
  EXPECT_TRUE(om->csps[1]->disposition == mojom::Disposition::REPORT_ONLY);
  EXPECT_TRUE(om->csps[1]->allowOverride);

  EXPECT_TRUE(om->csps[2]->policy == policy2);
  EXPECT_TRUE(om->csps[2]->disposition == mojom::Disposition::ENFORCE);
  EXPECT_FALSE(om->csps[2]->allowOverride);
}

TEST_F(OriginManifestParserTest, ParseOriginManifestWithCORSPreflights) {
  std::string origin = "https://a.com/";
  std::string version = "version1";
  std::string origin0 = "https://b.com/";
  std::string origin1 = "https://c.com/";
  std::string json =
      "{"
      "\"cors-preflights\": {"
      "\"no-credentials\": {"
      "\"origins\": \"*\","
      "},"
      "\"unsafe-include-credentials\": {"
      "\"origins\": [ \"" +
      origin0 + "\", \"" + origin1 +
      "\" ]"
      "},"
      "},"
      "}";

  mojom::OriginManifestPtr om =
      OriginManifestParser::Parse(origin, version, json);
  ASSERT_FALSE(om.Equals(nullptr));
  ASSERT_FALSE(om->corspreflights.Equals(nullptr));

  ASSERT_FALSE(om->corspreflights->nocredentials.Equals(nullptr));
  EXPECT_EQ(om->corspreflights->nocredentials->origins.size(), 1ul);
  EXPECT_TRUE(om->corspreflights->nocredentials->origins[0] == "*");

  ASSERT_FALSE(om->corspreflights->withcredentials.Equals(nullptr));
  EXPECT_EQ(om->corspreflights->withcredentials->origins.size(), 2ul);
  EXPECT_TRUE(om->corspreflights->withcredentials->origins[0] == origin0);
  EXPECT_TRUE(om->corspreflights->withcredentials->origins[1] == origin1);
}

}  // namespace content
