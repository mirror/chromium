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
#include "base/optional.h"
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

dnr_api::Rule CreateGenericParsedRule() {
  dnr_api::Rule rule;
  rule.id = 0;
  rule.condition.url_filter = base::MakeUnique<std::string>("filter");
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
    std::unique_ptr<dnr_api::Rule> rule =
        base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->id = cases[i].id;

    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(indexed_rule.id, base::checked_cast<uint32_t>(cases[i].id));
  }
}

TEST_F(IndexedRuleTest, PriorityParsing) {
  struct {
    std::unique_ptr<int> priority;
    const ParseResult expected_result;
    base::Optional<uint32_t> expected_priority;
  } cases[] = {
      {base::MakeUnique<int>(kMinValidPriority - 1),
       ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY, base::nullopt},
      {base::MakeUnique<int>(kMinValidPriority), ParseResult::SUCCESS,
       kMinValidPriority},
      {base::MakeUnique<int>(kMinValidPriority + 1), ParseResult::SUCCESS,
       kMinValidPriority + 1},
      {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY, base::nullopt},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule =
        base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->priority = std::move(cases[i].priority);
    rule->action.type = dnr_api::RULE_ACTION_TYPE_REDIRECT;
    rule->action.redirect_url = base::MakeUnique<std::string>("google.com");
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS) {
      ASSERT_TRUE(cases[i].expected_priority);
      EXPECT_EQ(*cases[i].expected_priority, indexed_rule.priority);
    }
  }

  // Ensure priority is ignored for non-redirect rules.
  {
    std::unique_ptr<dnr_api::Rule> rule =
        base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->priority = base::MakeUnique<int>(5);
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);
    EXPECT_EQ(ParseResult::SUCCESS, result);
    EXPECT_EQ(static_cast<uint32_t>(kDefaultPriority), indexed_rule.priority);
  }
}

TEST_F(IndexedRuleTest, OptionsParsing) {
  using DomainType = dnr_api::DomainType;
  using RuleActionType = dnr_api::RuleActionType;
  struct {
    const DomainType domain_type;
    const RuleActionType action_type;
    const bool url_filter_is_case_sensitive;
    const uint8_t expected_options;
  } cases[] = {
      {DomainType::DOMAIN_TYPE_NONE, RuleActionType::RULE_ACTION_TYPE_BLOCK,
       false,
       flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY |
           flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY},
      {DomainType::DOMAIN_TYPE_ANY, RuleActionType::RULE_ACTION_TYPE_BLOCK,
       true,
       flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY |
           flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY |
           flat_rule::OptionFlag_IS_MATCH_CASE},
      {DomainType::DOMAIN_TYPE_FIRST_PARTY,
       RuleActionType::RULE_ACTION_TYPE_WHITELIST, true,
       flat_rule::OptionFlag_IS_WHITELIST |
           flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY |
           flat_rule::OptionFlag_IS_MATCH_CASE},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule =
        base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->condition.domain_type = cases[i].domain_type;
    rule->action.type = cases[i].action_type;
    rule->condition.url_filter_is_case_sensitive =
        base::MakeUnique<bool>(cases[i].url_filter_is_case_sensitive);
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);
    EXPECT_EQ(ParseResult::SUCCESS, result);
    EXPECT_EQ(cases[i].expected_options, indexed_rule.options);
  }
}

TEST_F(IndexedRuleTest, ElementTypesParsing) {
  using ResourceType = dnr_api::ResourceType;
  using ResourceTypeVec = std::vector<ResourceType>;

  struct {
    std::unique_ptr<ResourceTypeVec> resource_types;
    std::unique_ptr<ResourceTypeVec> excluded_resource_types;
    ParseResult expected_result;
    uint16_t expected_element_types;
  } cases[] = {
      {nullptr, nullptr, ParseResult::SUCCESS, flat_rule::ElementType_ANY},
      {nullptr,
       base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT})),
       ParseResult::SUCCESS,
       flat_rule::ElementType_ANY & ~flat_rule::ElementType_SCRIPT},
      {base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT,
                            ResourceType::RESOURCE_TYPE_IMAGE})),
       nullptr, ParseResult::SUCCESS,
       flat_rule::ElementType_SCRIPT | flat_rule::ElementType_IMAGE},
      {base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT,
                            ResourceType::RESOURCE_TYPE_IMAGE})),
       base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT})),
       ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED,
       flat_rule::ElementType_NONE},
      {nullptr,
       base::MakeUnique<ResourceTypeVec>(ResourceTypeVec(
           {ResourceType::RESOURCE_TYPE_SUB_FRAME,
            ResourceType::RESOURCE_TYPE_STYLESHEET,
            ResourceType::RESOURCE_TYPE_SCRIPT,
            ResourceType::RESOURCE_TYPE_IMAGE, ResourceType::RESOURCE_TYPE_FONT,
            ResourceType::RESOURCE_TYPE_OBJECT,
            ResourceType::RESOURCE_TYPE_XMLHTTPREQUEST,
            ResourceType::RESOURCE_TYPE_PING, ResourceType::RESOURCE_TYPE_MEDIA,
            ResourceType::RESOURCE_TYPE_WEBSOCKET,
            ResourceType::RESOURCE_TYPE_OTHER})),
       ParseResult::ERROR_RULE_NOT_MEANINGFUL, flat_rule::ElementType_NONE},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule =
        base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
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
    std::string input_url_filter;
    flat_rule::UrlPatternType expected_url_pattern_type;
    flat_rule::AnchorType expected_anchor_left;
    flat_rule::AnchorType expected_anchor_right;
    std::string expected_url_pattern;
    ParseResult expected_result;
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
    std::unique_ptr<dnr_api::Rule> rule =
        base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->condition.url_filter =
        base::MakeUnique<std::string>(cases[i].input_url_filter);
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    EXPECT_EQ(cases[i].expected_url_pattern_type,
              indexed_rule.url_pattern_type);
    EXPECT_EQ(cases[i].expected_anchor_left, indexed_rule.anchor_left);
    EXPECT_EQ(cases[i].expected_anchor_right, indexed_rule.anchor_right);
    EXPECT_EQ(cases[i].expected_url_pattern, indexed_rule.url_pattern);
  }
}

TEST_F(IndexedRuleTest, DomainsParsing) {
  std::unique_ptr<dnr_api::Rule> rule =
      base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
  rule->condition.domains =
      base::MakeUnique<std::vector<std::string>>(std::vector<std::string>(
          {"http://b.com", "http://a.com", "http://b.com"}));
  rule->condition.excluded_domains =
      base::MakeUnique<std::vector<std::string>>(std::vector<std::string>(
          {"http://google.com", "http://yahoo.com", "http://yahoo.com"}));
  IndexedRule indexed_rule;
  ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);
  EXPECT_EQ(ParseResult::SUCCESS, result);
  EXPECT_EQ(indexed_rule.domains,
            std::vector<std::string>({"http://a.com", "http://b.com"}));
  EXPECT_EQ(
      indexed_rule.excluded_domains,
      std::vector<std::string>({"http://google.com", "http://yahoo.com"}));
}

TEST_F(IndexedRuleTest, RedirectUrlParsing) {
  struct {
    std::unique_ptr<std::string> redirect_url;
    const ParseResult expected_result;
    base::Optional<std::string> expected_redirect_url;
  } cases[] = {
      {base::MakeUnique<std::string>(""), ParseResult::ERROR_EMPTY_REDIRECT_URL,
       base::nullopt},
      {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_URL, base::nullopt},
      {base::MakeUnique<std::string>("google.com"), ParseResult::SUCCESS,
       std::string("google.com")},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule =
        base::MakeUnique<dnr_api::Rule>(CreateGenericParsedRule());
    rule->action.redirect_url = std::move(cases[i].redirect_url);
    rule->action.type = dnr_api::RULE_ACTION_TYPE_REDIRECT;
    rule->priority = base::MakeUnique<int>(kMinValidPriority);
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS) {
      ASSERT_TRUE(cases[i].expected_redirect_url);
      EXPECT_EQ(*cases[i].expected_redirect_url, indexed_rule.redirect_url);
    }
  }
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace  extensions
