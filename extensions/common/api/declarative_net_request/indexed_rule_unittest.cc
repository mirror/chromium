// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/indexed_rule.h"

#include <memory>
#include <utility>

#include "base/format_macros.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace flat_rule = ::url_pattern_index::flat;
namespace dnr_api = ::extensions::api::declarative_net_request;
using IndexedRuleTest = ::testing::Test;
using base::MakeUnique;
using std::unique_ptr;

dnr_api::Rule CreateGenericParsedRule() {
  dnr_api::Rule rule;
  rule.id = 0;
  rule.condition.url_filter = MakeUnique<std::string>("filter");
  rule.action.type = dnr_api::RULE_ACTION_TYPE_BLOCK;
  return rule;
}

TEST_F(IndexedRuleTest, IDParsing) {
  struct {
    const int id;
    const ParseResult expected_result;
  } cases[] = {
      {kMinValidID - 1, ParseResult::ERROR_INVALID_RULE_ID},
      {kMinValidID, ParseResult::SUCCESS},
      {kMinValidID + 1, ParseResult::SUCCESS},
  };
  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->id = cases[i].id;

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(base::checked_cast<uint32_t>(cases[i].id), indexed_rule.id);
  }
}

TEST_F(IndexedRuleTest, PriorityParsing) {
  struct {
    unique_ptr<int> priority;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const uint32_t expected_priority;
  } cases[] = {
      {MakeUnique<int>(kMinValidPriority - 1),
       ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY, kDefaultPriority},
      {MakeUnique<int>(kMinValidPriority), ParseResult::SUCCESS,
       kMinValidPriority},
      {MakeUnique<int>(kMinValidPriority + 1), ParseResult::SUCCESS,
       kMinValidPriority + 1},
      {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY,
       kDefaultPriority},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->priority = std::move(cases[i].priority);
    rule->action.type = dnr_api::RULE_ACTION_TYPE_REDIRECT;
    rule->action.redirect_url = MakeUnique<std::string>("google.com");

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(cases[i].expected_priority, indexed_rule.priority);
  }

  // Ensure priority is ignored for non-redirect rules.
  {
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->priority = MakeUnique<int>(5);
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);
    EXPECT_EQ(ParseResult::SUCCESS, result);
    EXPECT_EQ(static_cast<uint32_t>(kDefaultPriority), indexed_rule.priority);
  }
}

TEST_F(IndexedRuleTest, OptionsParsing) {
  struct {
    const dnr_api::DomainType domain_type;
    const dnr_api::RuleActionType action_type;
    unique_ptr<bool> is_url_filter_case_sensitive;
    const uint8_t expected_options;
  } cases[] = {
      {dnr_api::DOMAIN_TYPE_NONE, dnr_api::RULE_ACTION_TYPE_BLOCK, nullptr,
       flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY |
           flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY},
      {dnr_api::DOMAIN_TYPE_FIRSTPARTY, dnr_api::RULE_ACTION_TYPE_WHITELIST,
       base::MakeUnique<bool>(true),
       flat_rule::OptionFlag_IS_WHITELIST |
           flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY |
           flat_rule::OptionFlag_IS_MATCH_CASE},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->condition.domain_type = cases[i].domain_type;
    rule->action.type = cases[i].action_type;
    rule->condition.is_url_filter_case_sensitive =
        std::move(cases[i].is_url_filter_case_sensitive);

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(ParseResult::SUCCESS, result);
    EXPECT_EQ(cases[i].expected_options, indexed_rule.options);
  }
}

TEST_F(IndexedRuleTest, ResourceTypesParsing) {
  using ResourceTypeVec = std::vector<dnr_api::ResourceType>;

  struct {
    unique_ptr<ResourceTypeVec> resource_types;
    unique_ptr<ResourceTypeVec> excluded_resource_types;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const uint16_t expected_element_types;
  } cases[] = {
      {nullptr, nullptr, ParseResult::SUCCESS, flat_rule::ElementType_ANY},
      {nullptr,
       MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({dnr_api::RESOURCE_TYPE_SCRIPT})),
       ParseResult::SUCCESS,
       flat_rule::ElementType_ANY & ~flat_rule::ElementType_SCRIPT},
      {MakeUnique<ResourceTypeVec>(ResourceTypeVec(
           {dnr_api::RESOURCE_TYPE_SCRIPT, dnr_api::RESOURCE_TYPE_IMAGE})),
       nullptr, ParseResult::SUCCESS,
       flat_rule::ElementType_SCRIPT | flat_rule::ElementType_IMAGE},
      {MakeUnique<ResourceTypeVec>(ResourceTypeVec(
           {dnr_api::RESOURCE_TYPE_SCRIPT, dnr_api::RESOURCE_TYPE_IMAGE})),
       MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({dnr_api::RESOURCE_TYPE_SCRIPT})),
       ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED,
       flat_rule::ElementType_NONE},
      {nullptr,
       MakeUnique<ResourceTypeVec>(ResourceTypeVec(
           {dnr_api::RESOURCE_TYPE_SUB_FRAME, dnr_api::RESOURCE_TYPE_STYLESHEET,
            dnr_api::RESOURCE_TYPE_SCRIPT, dnr_api::RESOURCE_TYPE_IMAGE,
            dnr_api::RESOURCE_TYPE_FONT, dnr_api::RESOURCE_TYPE_OBJECT,
            dnr_api::RESOURCE_TYPE_XMLHTTPREQUEST, dnr_api::RESOURCE_TYPE_PING,
            dnr_api::RESOURCE_TYPE_MEDIA, dnr_api::RESOURCE_TYPE_WEBSOCKET,
            dnr_api::RESOURCE_TYPE_OTHER})),
       ParseResult::ERROR_RULE_NOT_MEANINGFUL, flat_rule::ElementType_NONE},
      {MakeUnique<ResourceTypeVec>(ResourceTypeVec()),
       MakeUnique<ResourceTypeVec>(ResourceTypeVec()),
       ParseResult::ERROR_EMPTY_RESOURCE_TYPES_LIST,
       flat_rule::ElementType_NONE},
      {MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({dnr_api::RESOURCE_TYPE_SCRIPT})),
       MakeUnique<ResourceTypeVec>(ResourceTypeVec()), ParseResult::SUCCESS,
       flat_rule::ElementType_SCRIPT},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->condition.resource_types = std::move(cases[i].resource_types);
    rule->condition.excluded_resource_types =
        std::move(cases[i].excluded_resource_types);

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(cases[i].expected_element_types, indexed_rule.element_types);
  }
}

TEST_F(IndexedRuleTest, UrlFilterParsing) {
  struct {
    const std::string input_url_filter;

    // Only valid if |expected_result| is SUCCESS.
    const flat_rule::UrlPatternType expected_url_pattern_type;
    const flat_rule::AnchorType expected_anchor_left;
    const flat_rule::AnchorType expected_anchor_right;
    const std::string expected_url_pattern;

    const ParseResult expected_result;
  } cases[] = {
      {"", flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_NONE,
       flat_rule::AnchorType_NONE, "", ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"|", flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_BOUNDARY,
       flat_rule::AnchorType_NONE, "", ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"||", flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_SUBDOMAIN, flat_rule::AnchorType_NONE, "",
       ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"|||", flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_SUBDOMAIN, flat_rule::AnchorType_BOUNDARY, "",
       ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"|*|||", flat_rule::UrlPatternType_WILDCARDED,
       flat_rule::AnchorType_BOUNDARY, flat_rule::AnchorType_BOUNDARY, "*||",
       ParseResult::SUCCESS},
      {"|xyz|", flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_BOUNDARY, flat_rule::AnchorType_BOUNDARY, "xyz",
       ParseResult::SUCCESS},
      {"||x^yz", flat_rule::UrlPatternType_WILDCARDED,
       flat_rule::AnchorType_SUBDOMAIN, flat_rule::AnchorType_NONE, "x^yz",
       ParseResult::SUCCESS},
      {"||xyz|", flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_SUBDOMAIN, flat_rule::AnchorType_BOUNDARY, "xyz",
       ParseResult::SUCCESS},
      {"x*y|z", flat_rule::UrlPatternType_WILDCARDED,
       flat_rule::AnchorType_NONE, flat_rule::AnchorType_NONE, "x*y|z",
       ParseResult::SUCCESS},
      {"**^", flat_rule::UrlPatternType_WILDCARDED, flat_rule::AnchorType_NONE,
       flat_rule::AnchorType_NONE, "**^", ParseResult::SUCCESS},
      {"||google.com", flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_SUBDOMAIN, flat_rule::AnchorType_NONE,
       "google.com", ParseResult::SUCCESS},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->condition.url_filter =
        MakeUnique<std::string>(cases[i].input_url_filter);

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);
    if (result != ParseResult::SUCCESS)
      continue;

    EXPECT_EQ(cases[i].expected_result, result);
    EXPECT_EQ(cases[i].expected_url_pattern_type,
              indexed_rule.url_pattern_type);
    EXPECT_EQ(cases[i].expected_anchor_left, indexed_rule.anchor_left);
    EXPECT_EQ(cases[i].expected_anchor_right, indexed_rule.anchor_right);
    EXPECT_EQ(cases[i].expected_url_pattern, indexed_rule.url_pattern);
  }
}

TEST_F(IndexedRuleTest, DomainsParsing) {
  using DomainVec = std::vector<std::string>;
  struct {
    unique_ptr<DomainVec> domains;
    unique_ptr<DomainVec> excluded_domains;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const DomainVec expected_domains;
    const DomainVec expected_excluded_domains;
  } cases[] = {{nullptr, nullptr, ParseResult::SUCCESS, {}, {}},
               {MakeUnique<DomainVec>(DomainVec()),
                nullptr,
                ParseResult::ERROR_EMPTY_DOMAINS_LIST,
                {},
                {}},
               {nullptr,
                MakeUnique<DomainVec>(DomainVec()),
                ParseResult::SUCCESS,
                {},
                {}},
               {MakeUnique<DomainVec>(DomainVec({"a.com", "b.com", "a.com"})),
                MakeUnique<DomainVec>(DomainVec({"g.com", "a.com", "g.com"})),
                ParseResult::SUCCESS,
                {"a.com", "b.com"},
                {"a.com", "g.com"}}};

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->condition.domains = std::move(cases[i].domains);
    rule->condition.excluded_domains = std::move(cases[i].excluded_domains);

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS) {
      EXPECT_EQ(cases[i].expected_domains, indexed_rule.domains);
      EXPECT_EQ(cases[i].expected_excluded_domains,
                indexed_rule.excluded_domains);
    }
  }
}

TEST_F(IndexedRuleTest, RedirectUrlParsing) {
  struct {
    unique_ptr<std::string> redirect_url;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const std::string expected_redirect_url;
  } cases[] = {
      {MakeUnique<std::string>(""), ParseResult::ERROR_EMPTY_REDIRECT_URL, ""},
      {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_URL, ""},
      {MakeUnique<std::string>("google.com"), ParseResult::SUCCESS,
       "google.com"},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    unique_ptr<dnr_api::Rule> rule =
        MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->action.redirect_url = std::move(cases[i].redirect_url);
    rule->action.type = dnr_api::RULE_ACTION_TYPE_REDIRECT;
    rule->priority = MakeUnique<int>(kMinValidPriority);

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(cases[i].expected_redirect_url, indexed_rule.redirect_url);
  }
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace  extensions
