// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "content/browser/origin_manifest/origin_manifest_parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class OriginManifestParserTest : public testing::Test {};

// Note that we do not initialize the persistent store here since that
// is subject to testing the persistent store itself.

TEST_F(OriginManifestParserTest, ParsingSuccess) {
  // basically it should only create an Origin Manifest on valid JSON
  struct TestCase {
    std::string json;
    bool equalsNull;
  } cases[] = {
      {"", true},
      {"{ \"I am not valid\"", true},
      {"{ }", false},
      {"{ \"valid\": \"JSON\", \"but\": [\"unknown\", \"structure\"] }", false},
      {"{ \"tls\": {}, \"tls\": {} }", false}};

  for (const auto& test : cases)
    EXPECT_EQ(OriginManifestParser::Parse(test.json).get() == nullptr,
              test.equalsNull);
}

TEST_F(OriginManifestParserTest, ParseCSPWithUnexpectedSyntax) {
  // CSP value not a list
  std::string json = "{ \"content-security-policy\": 42 }";
  std::unique_ptr<blink::OriginManifest> om = OriginManifestParser::Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(false).size(), 0ul);

  // not a list of objects but directly the CSP attributes
  json =
      "{ \"content-security-policy\": [ \"this is not the content\", \"you are "
      "looking for\" ] }";
  om = OriginManifestParser::Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(false).size(), 0ul);

  // CSP alongside unknown attributes should be fine
  json =
      "{ "
      "\"unknown\": 42,"
      "\"content-security-policy\": ["
      "  { \"policy\": \"default-src 'none'\" },"
      "],"
      "\"hardcore\": \"unknown\","
      " }";
  om = OriginManifestParser::Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(false).size(), 1ul);

  // containing unknown attributes should be fine
  json =
      "{ "
      "\"content-security-policy\": [ {"
      "  \"unknown\": 42,"
      "  \"policy\": \"default-src 'none'\","
      "  \"hardcore\": \"unknown\","
      " } ],"
      "}";
  om = OriginManifestParser::Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(false).size(), 1ul);
  EXPECT_EQ(om->GetContentSecurityPolicies(false)[0].policy,
            "default-src 'none'");

  // multiple CSPs
  json =
      "{ \"content-security-policy\": ["
      "{ \"policy\": \"default-src 'none'\" },"
      "{ \"policy\": \"default-src 'none'\" }"
      "] }";
  om = OriginManifestParser::Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(false).size(), 2ul);
}

TEST_F(OriginManifestParserTest, ParseCSPWithMissingAttributes) {
  // invalid values are tested separately below, no need to re-test it here
  struct TestCase {
    std::string policy;
    std::string disposition;
    std::string allow_override;
    std::string expected_policy;
    blink::OriginManifest::ContentSecurityPolicyType expected_disposition;
    bool expected_allow_override;
  } cases[] = {
      {"", "", "", "",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, false},
      {"", "report-only", "true", "",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, true},
      {"", "", "true", "",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, true},
      {"", "report-only", "", "",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, false},
      {"default-src 'none'", "", "true", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, true},
      {"default-src 'none'", "", "", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, false},
      {"default-src 'none'", "report-only", "", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, false},
      {"default-src 'none'", "report-only", "true", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, true},
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

    std::unique_ptr<blink::OriginManifest> om =
        OriginManifestParser::Parse(str_stream.str());

    ASSERT_FALSE(om.get() == nullptr);
    std::vector<blink::OriginManifest::ContentSecurityPolicy>
        baseline_fallback = om->GetContentSecurityPolicies(false);
    ASSERT_EQ(baseline_fallback.size(), 1ul);

    EXPECT_EQ(baseline_fallback[0].policy, test.expected_policy);
    EXPECT_EQ(baseline_fallback[0].disposition, test.expected_disposition);

    // if override is not allowed it is a fallback and baseline should be empty
    std::vector<blink::OriginManifest::ContentSecurityPolicy> baseline =
        om->GetContentSecurityPolicies(true);
    EXPECT_EQ(baseline.size(), (test.expected_allow_override) ? 1ul : 0ul);
  }
}

TEST_F(OriginManifestParserTest, GetDirectiveType) {
  OriginManifestParser::DirectiveType type =
      OriginManifestParser::GetDirectiveType("content-security-policy");
  EXPECT_EQ(type, OriginManifestParser::DirectiveType::kContentSecurityPolicy);

  type = OriginManifestParser::GetDirectiveType("asdf");
  EXPECT_EQ(type, OriginManifestParser::DirectiveType::kUnknown);
}

TEST_F(OriginManifestParserTest, GetCSPDisposition) {
  blink::OriginManifest::ContentSecurityPolicyType type =
      OriginManifestParser::GetCSPDisposition("enforce");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kEnforce);

  type = OriginManifestParser::GetCSPDisposition("report-only");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kReport);

  type = OriginManifestParser::GetCSPDisposition("");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kEnforce);

  type = OriginManifestParser::GetCSPDisposition("asdf");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kEnforce);
}

TEST_F(OriginManifestParserTest, GetCSPActivationType) {
  EXPECT_EQ(OriginManifestParser::GetCSPActivationType(true),
            blink::ActivationType::kBaseline);
  EXPECT_EQ(OriginManifestParser::GetCSPActivationType(false),
            blink::ActivationType::kFallback);
}

TEST_F(OriginManifestParserTest, ParseContentSecurityPolicy) {
  // Wrong structure
  blink::OriginManifest om;
  base::Value dict(base::Value::Type::DICTIONARY);
  OriginManifestParser::ParseContentSecurityPolicy(&om, std::move(dict));
  EXPECT_EQ(om.GetContentSecurityPolicies(false).size(), 0ul);

  // empty list
  om = blink::OriginManifest();
  base::Value list(base::Value::Type::LIST);
  OriginManifestParser::ParseContentSecurityPolicy(&om, std::move(list));
  EXPECT_EQ(om.GetContentSecurityPolicies(false).size(), 0ul);

  // list with completely unexpected element
  om = blink::OriginManifest();
  list = base::Value(base::Value::Type::LIST);
  list.GetList().push_back(base::Value(42));
  OriginManifestParser::ParseContentSecurityPolicy(&om, std::move(list));
  EXPECT_EQ(om.GetContentSecurityPolicies(false).size(), 0ul);

  // list with 1 valid, 2 invalid CSPs -> 3 policies (we don't validate CSPs)
  om = blink::OriginManifest();
  list = base::Value(base::Value::Type::LIST);

  std::string policy0 = "default-src 'none'";
  std::string policy1 = "";
  std::string policy2 = "asdf";

  base::Value csp0(base::Value::Type::DICTIONARY);
  csp0.SetKey("policy", base::Value(policy0));
  csp0.SetKey("disposition", base::Value("report-only"));
  csp0.SetKey("allow-override", base::Value(true));
  list.GetList().push_back(std::move(csp0));

  base::Value csp1(base::Value::Type::DICTIONARY);
  csp1.SetKey("policy", base::Value(policy1));
  csp1.SetKey("disposition", base::Value(23));
  csp1.SetKey("allow-override", base::Value(42));
  list.GetList().push_back(std::move(csp1));

  base::Value csp2(base::Value::Type::DICTIONARY);
  csp2.SetKey("policy", base::Value(policy2));
  list.GetList().push_back(std::move(csp2));

  OriginManifestParser::ParseContentSecurityPolicy(&om, std::move(list));

  std::vector<blink::OriginManifest::ContentSecurityPolicy> baseline =
      om.GetContentSecurityPolicies(true);
  ASSERT_EQ(baseline.size(), 1ul);
  EXPECT_TRUE(baseline[0].policy == policy0);

  // we know that every CSP s a dictionary and has a unique policy here.
  std::vector<blink::OriginManifest::ContentSecurityPolicy> baseline_fallback =
      om.GetContentSecurityPolicies(false);
  ASSERT_EQ(baseline_fallback.size(), 3ul);
  std::vector<blink::OriginManifest::ContentSecurityPolicy>::iterator it;
  for (it = baseline_fallback.begin(); it != baseline_fallback.end(); ++it) {
    if (it->policy == policy0)
      break;
  }
  ASSERT_FALSE(it == baseline_fallback.end());
  baseline_fallback.erase(it);
  if (baseline_fallback[0].policy == policy1)
    EXPECT_TRUE(baseline_fallback[1].policy == policy2);
  else
    EXPECT_TRUE(baseline_fallback[0].policy == policy1);
}

}  // namespace content
