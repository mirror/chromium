// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "content/browser/origin_manifest/origin_manifest_parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

class OriginManifestParserTest : public testing::Test {};

// Note that we do not initialize the persistent store here since that
// is subject to testing the persistent store itself.

TEST_F(OriginManifestParserTest, ParsingSuccess) {
  // basically it should only create an Origin Manifest on valid JSON
  url::Origin origin(GURL("https://a.com/"));
  std::string version = "version1";
  struct TestCase {
    std::string json;
    bool equalsNull;
  } cases[] = {
      {"", true},
      {"{ \"I am not valid\"", true},
      {"{ }", false},
      {"{ \"valid\": \"JSON\", \"but\": [\"unknown\", \"structure\"] }", false},
      {"{ \"hsts\": {}, \"hsts\": {} }", false}};

  for (const auto& test : cases) {
    EXPECT_EQ(
        OriginManifestParser::Parse(origin, version, test.json).Equals(nullptr),
        test.equalsNull);
  }
}

TEST_F(OriginManifestParserTest, ParseCSPWithUnexpectedSyntax) {
  url::Origin origin(GURL("https://a.com/"));
  std::string version = "version1";

  // CSP value not a list
  std::string json = "{ \"content-security-policy\": 42 }";
  blink::mojom::OriginManifestPtr om =
      OriginManifestParser::Parse(origin, version, json);
  ASSERT_FALSE(om.Equals(nullptr));
  EXPECT_EQ(om->csps.size(), 0ul);

  // not a list of objects but directly the CSP attributes
  json =
      "{ \"content-security-policy\": [ \"this is not the content\", \"you are "
      "looking for\" ] }";
  om = OriginManifestParser::Parse(origin, version, json);
  ASSERT_FALSE(om.Equals(nullptr));
  EXPECT_EQ(om->csps.size(), 0ul);

  // multiple CSPs
  json =
      "{ \"content-security-policy\": ["
      "{ \"policy\": \"default-src 'none'\" },"
      "{ \"policy\": \"default-src 'none'\" }"
      "] }";
  om = OriginManifestParser::Parse(origin, version, json);
  ASSERT_FALSE(om.Equals(nullptr));
  EXPECT_EQ(om->csps.size(), 2ul);
}

TEST_F(OriginManifestParserTest, ParseCSPWithMissingAttributes) {
  url::Origin origin(GURL("https://a.com/"));
  std::string version = "version1";

  // invalid values are tested separately below, no need to re-test it here
  struct TestCase {
    std::string policy;
    std::string disposition;
    std::string allow_override;
    std::string expected_policy;
    blink::WebContentSecurityPolicyType expected_disposition;
    bool expected_allow_override;
  } cases[] = {
      {"", "", "", "",
       blink::WebContentSecurityPolicyType::
           kWebContentSecurityPolicyTypeEnforce,
       false},
      {"", "report-only", "true", "",
       blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport,
       true},
      {"", "", "true", "",
       blink::WebContentSecurityPolicyType::
           kWebContentSecurityPolicyTypeEnforce,
       true},
      {"", "report-only", "", "",
       blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport,
       false},
      {"default-src 'none'", "", "true", "default-src 'none'",
       blink::WebContentSecurityPolicyType::
           kWebContentSecurityPolicyTypeEnforce,
       true},
      {"default-src 'none'", "", "", "default-src 'none'",
       blink::WebContentSecurityPolicyType::
           kWebContentSecurityPolicyTypeEnforce,
       false},
      {"default-src 'none'", "report-only", "", "default-src 'none'",
       blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport,
       false},
      {"default-src 'none'", "report-only", "true", "default-src 'none'",
       blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport,
       true},
  };

  for (const auto& test : cases) {
    std::ostringstream str_stream;
    str_stream << "{ \"content-security-policy\": [ {";
    if (test.policy.length() > 0)
      str_stream << "\"policy\": \"" << test.policy + "\", ";
    if (test.disposition.length() > 0)
      str_stream << "\"disposition\": \"" << test.disposition + "\", ";
    if (test.allow_override.length() > 0)
      str_stream << "\"allow-override\": " << test.allow_override + ", ";
    str_stream << "}, ] }";

    blink::mojom::OriginManifestPtr om =
        OriginManifestParser::Parse(origin, version, str_stream.str());

    ASSERT_FALSE(om.Equals(nullptr));
    ASSERT_EQ(om->csps.size(), 1ul);
    ASSERT_EQ(om->csps[0]->policy, test.expected_policy);
    ASSERT_EQ(om->csps[0]->disposition, test.expected_disposition);
    ASSERT_EQ(om->csps[0]->allowOverride, test.expected_allow_override);
  }
}

TEST_F(OriginManifestParserTest, ParseOriginManifestWithCORSPreflights) {
  url::Origin origin(GURL("https://a.com/"));
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

  blink::mojom::OriginManifestPtr om =
      OriginManifestParser::Parse(origin, version, json);
  ASSERT_FALSE(om.Equals(nullptr));
  ASSERT_FALSE(om->corspreflights.Equals(nullptr));

  ASSERT_FALSE(om->corspreflights->nocredentials.Equals(nullptr));
  EXPECT_EQ(om->corspreflights->nocredentials->origins.size(), 0ul);
  EXPECT_TRUE(om->corspreflights->nocredentials->allowAll);

  ASSERT_FALSE(om->corspreflights->withcredentials.Equals(nullptr));
  EXPECT_EQ(om->corspreflights->withcredentials->origins.size(), 2ul);
  EXPECT_TRUE(om->corspreflights->withcredentials->origins[0].IsSameOriginWith(
      url::Origin(GURL(origin0))));
  EXPECT_TRUE(om->corspreflights->withcredentials->origins[1].IsSameOriginWith(
      url::Origin(GURL(origin1))));
}

TEST_F(OriginManifestParserTest, ParseDisposition) {
  std::unique_ptr<base::Value> value;

  value.reset(new base::Value("enforce"));
  EXPECT_EQ(OriginManifestParser::ParseDisposition(value.get()),
            blink::WebContentSecurityPolicyType::
                kWebContentSecurityPolicyTypeEnforce);
  value.reset(new base::Value("report-only"));
  EXPECT_EQ(
      OriginManifestParser::ParseDisposition(value.get()),
      blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport);
  value.reset(new base::Value(""));
  EXPECT_EQ(OriginManifestParser::ParseDisposition(value.get()),
            blink::WebContentSecurityPolicyType::
                kWebContentSecurityPolicyTypeEnforce);
  value.reset(new base::Value("asdf"));
  EXPECT_EQ(OriginManifestParser::ParseDisposition(value.get()),
            blink::WebContentSecurityPolicyType::
                kWebContentSecurityPolicyTypeEnforce);
}

TEST_F(OriginManifestParserTest, ParseContentSecurityPolicies) {
  std::vector<base::Value> list;
  EXPECT_EQ(OriginManifestParser::ParseContentSecurityPolicies(std::move(list))
                .size(),
            0ul);

  // completely unexpected element
  list.push_back(base::Value(42));
  EXPECT_EQ(OriginManifestParser::ParseContentSecurityPolicies(std::move(list))
                .size(),
            0ul);

  // list with 1 valid, 2 invalid CSPs -> 3 policies (we don't validate the
  // CSPs)
  base::Value csp0(base::Value::Type::DICTIONARY);
  csp0.SetKey("policy", base::Value("default-src 'none'"));
  csp0.SetKey("disposition", base::Value("report-only"));
  csp0.SetKey("allow-override", base::Value(true));
  list.push_back(std::move(csp0));

  base::Value csp1(base::Value::Type::DICTIONARY);
  csp1.SetKey("policy", base::Value());
  csp1.SetKey("disposition", base::Value(23));
  csp1.SetKey("allow-override", base::Value(42));
  list.push_back(std::move(csp1));

  base::Value csp2(base::Value::Type::DICTIONARY);
  list.push_back(std::move(csp2));

  std::vector<blink::mojom::ContentSecurityPolicyPtr> parsed =
      OriginManifestParser::ParseContentSecurityPolicies(std::move(list));
  EXPECT_EQ(parsed.size(), 3ul);

  EXPECT_TRUE(parsed[0]->policy == "default-src 'none'");
  EXPECT_EQ(
      parsed[0]->disposition,
      blink::WebContentSecurityPolicyType::kWebContentSecurityPolicyTypeReport);
  EXPECT_TRUE(parsed[0]->allowOverride);

  EXPECT_TRUE(parsed[1]->policy == "");
  EXPECT_EQ(parsed[1]->disposition, blink::WebContentSecurityPolicyType::
                                        kWebContentSecurityPolicyTypeEnforce);
  EXPECT_FALSE(parsed[1]->allowOverride);

  EXPECT_TRUE(parsed[2]->policy == "");
  EXPECT_EQ(parsed[2]->disposition, blink::WebContentSecurityPolicyType::
                                        kWebContentSecurityPolicyTypeEnforce);
  EXPECT_FALSE(parsed[2]->allowOverride);
}

TEST_F(OriginManifestParserTest, ParseCORSPreflight) {
  base::DictionaryValue cors;
  cors.SetKey("foo", base::Value(42));

  blink::mojom::CORSPreflightPtr parsed =
      OriginManifestParser::ParseCORSPreflight(&cors);
  EXPECT_TRUE(parsed->nocredentials.Equals(nullptr));
  EXPECT_TRUE(parsed->withcredentials.Equals(nullptr));

  cors.RemovePath({"foo"});
  cors.SetKey("no-credentials", base::Value(42));
  cors.SetKey("unsafe-include-credentials", base::Value(42));

  parsed = OriginManifestParser::ParseCORSPreflight(&cors);
  EXPECT_TRUE(parsed->nocredentials.Equals(nullptr));
  EXPECT_TRUE(parsed->withcredentials.Equals(nullptr));

  base::Value dict0(base::Value::Type::DICTIONARY);
  dict0.SetKey("origins", base::Value(42));
  cors.SetKey("no-credentials", std::move(dict0));
  base::Value dict1(base::Value::Type::DICTIONARY);
  dict1.SetKey("origins", base::Value(42));
  cors.SetKey("unsafe-include-credentials", std::move(dict1));

  parsed = OriginManifestParser::ParseCORSPreflight(&cors);
  ASSERT_FALSE(parsed->nocredentials.Equals(nullptr));
  EXPECT_FALSE(parsed->nocredentials->allowAll);
  EXPECT_EQ(parsed->nocredentials->origins.size(), 0ul);
  ASSERT_FALSE(parsed->withcredentials.Equals(nullptr));
  EXPECT_FALSE(parsed->withcredentials->allowAll);
  EXPECT_EQ(parsed->withcredentials->origins.size(), 0ul);
}

TEST_F(OriginManifestParserTest, ParseCORSOrigins) {
  std::unique_ptr<base::Value> value;
  value.reset(new base::Value(23));
  blink::mojom::CORSOriginsPtr corsorigins =
      OriginManifestParser::ParseCORSOrigins(value.get());
  ASSERT_FALSE(corsorigins.Equals(nullptr));
  EXPECT_FALSE(corsorigins->allowAll);
  EXPECT_EQ(corsorigins->origins.size(), 0ul);

  value.reset(new base::Value("*"));
  corsorigins = OriginManifestParser::ParseCORSOrigins(value.get());
  EXPECT_TRUE(corsorigins->allowAll);
  EXPECT_EQ(corsorigins->origins.size(), 0ul);

  value.reset(new base::Value(base::Value::Type::LIST));
  corsorigins = OriginManifestParser::ParseCORSOrigins(value.get());
  EXPECT_FALSE(corsorigins->allowAll);
  EXPECT_EQ(corsorigins->origins.size(), 0ul);

  value.reset(new base::Value(base::Value::Type::LIST));
  value->GetList().push_back(base::Value("*"));
  corsorigins = OriginManifestParser::ParseCORSOrigins(value.get());
  EXPECT_TRUE(corsorigins->allowAll);
  EXPECT_EQ(corsorigins->origins.size(), 0ul);

  value.reset(new base::Value(base::Value::Type::LIST));
  value->GetList().push_back(base::Value("a.com"));
  value->GetList().push_back(base::Value("https://b.com"));
  corsorigins = OriginManifestParser::ParseCORSOrigins(value.get());
  EXPECT_FALSE(corsorigins->allowAll);
  EXPECT_EQ(corsorigins->origins.size(), 1ul);

  // the double elements are not really wanted but it turns out it works
  value.reset(new base::Value(base::Value::Type::LIST));
  value->GetList().push_back(base::Value("https://b.com"));
  value->GetList().push_back(base::Value("*"));
  value->GetList().push_back(base::Value("https://b.com"));
  corsorigins = OriginManifestParser::ParseCORSOrigins(value.get());
  EXPECT_TRUE(corsorigins->allowAll);
  EXPECT_EQ(corsorigins->origins.size(), 2ul);
}

}  // namespace content
