// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/indexed_rule.h"

#include "base/format_macros.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace declarative_net_request {
namespace {

using IndexedRuleTest = testing::Test;

ParsedRule CreateGenericParsedRule() {
  ParsedRule rule;
  rule.id = 0;
  rule.action.type = api::declarative_net_request::RULE_ACTION_TYPE_BLOCK;
  rule.condition.url_filter = base::MakeUnique<std::string>("filter");
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
    std::unique_ptr<ParsedRule> rule =
        base::MakeUnique<ParsedRule>(CreateGenericParsedRule());
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
  } cases[] = {
      {base::MakeUnique<int>(kMinValidPriority - 1),
       ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY},
      {base::MakeUnique<int>(kMinValidPriority), ParseResult::SUCCESS},
      {base::MakeUnique<int>(kMinValidPriority + 1), ParseResult::SUCCESS},
      {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<ParsedRule> rule =
        base::MakeUnique<ParsedRule>(CreateGenericParsedRule());
    // Copy the priority for later use.
    std::unique_ptr<int> priority_copy =
        cases[i].priority ? base::MakeUnique<int>(*cases[i].priority) : nullptr;
    rule->priority = std::move(cases[i].priority);
    rule->action.type = api::declarative_net_request::RULE_ACTION_TYPE_REDIRECT;
    rule->action.redirect_url = base::MakeUnique<std::string>("google.com");
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(indexed_rule.priority,
                base::checked_cast<uint32_t>(*priority_copy));
  }
}

TEST_F(IndexedRuleTest, OptionsParsing) {
  using DomainType = api::declarative_net_request::DomainType;
  using RuleActionType = api::declarative_net_request::RuleActionType;
  struct {
    const DomainType domain_type;
    const RuleActionType action_type;
    const bool url_filter_is_case_sensitive;
    const uint8_t expected_options;
  } cases[] = {
      {DomainType::DOMAIN_TYPE_NONE, RuleActionType::RULE_ACTION_TYPE_BLOCK,
       false,
       OptionFlag::OptionFlag_APPLIES_TO_THIRD_PARTY |
           OptionFlag::OptionFlag_APPLIES_TO_FIRST_PARTY},
      {DomainType::DOMAIN_TYPE_ANY, RuleActionType::RULE_ACTION_TYPE_BLOCK,
       true,
       OptionFlag::OptionFlag_APPLIES_TO_THIRD_PARTY |
           OptionFlag::OptionFlag_APPLIES_TO_FIRST_PARTY |
           OptionFlag::OptionFlag_IS_MATCH_CASE},
      {DomainType::DOMAIN_TYPE_FIRST_PARTY,
       RuleActionType::RULE_ACTION_TYPE_WHITELIST, true,
       OptionFlag::OptionFlag_IS_WHITELIST |
           OptionFlag::OptionFlag_APPLIES_TO_FIRST_PARTY |
           OptionFlag::OptionFlag_IS_MATCH_CASE},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<ParsedRule> rule =
        base::MakeUnique<ParsedRule>(CreateGenericParsedRule());
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
  using ResourceType = api::declarative_net_request::ResourceType;
  using ResourceTypeVec = std::vector<ResourceType>;
  struct {
    std::unique_ptr<ResourceTypeVec> resource_types;
    std::unique_ptr<ResourceTypeVec> excluded_resource_types;
    ParseResult expected_result;
    uint16_t expected_element_types;
  } cases[] = {
      {nullptr, nullptr, ParseResult::SUCCESS, ElementType::ElementType_ANY},
      {nullptr,
       base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT})),
       ParseResult::SUCCESS,
       ElementType::ElementType_ANY & ~ElementType::ElementType_SCRIPT},
      {base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT,
                            ResourceType::RESOURCE_TYPE_IMAGE})),
       nullptr, ParseResult::SUCCESS,
       ElementType::ElementType_SCRIPT | ElementType::ElementType_IMAGE},
      {base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT,
                            ResourceType::RESOURCE_TYPE_IMAGE})),
       base::MakeUnique<ResourceTypeVec>(
           ResourceTypeVec({ResourceType::RESOURCE_TYPE_SCRIPT})),
       ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED,
       ElementType::ElementType_NONE},
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
       ParseResult::ERROR_RULE_NOT_MEANINGFUL, ElementType::ElementType_NONE},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<ParsedRule> rule =
        base::MakeUnique<ParsedRule>(CreateGenericParsedRule());
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
    const UrlPatternType expected_url_pattern_type;
    const AnchorType expected_anchor_left;
    const AnchorType expected_anchor_right;
    const std::string expected_url_pattern;
    const ParseResult expected_result;
  } cases[] = {
      {"", UrlPatternType::UrlPatternType_SUBSTRING,
       AnchorType::AnchorType_NONE, AnchorType::AnchorType_NONE, "",
       ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"|", UrlPatternType::UrlPatternType_SUBSTRING,
       AnchorType::AnchorType_BOUNDARY, AnchorType::AnchorType_NONE, "",
       ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"||", UrlPatternType::UrlPatternType_SUBSTRING,
       AnchorType::AnchorType_SUBDOMAIN, AnchorType::AnchorType_NONE, "",
       ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"|||", UrlPatternType::UrlPatternType_SUBSTRING,
       AnchorType::AnchorType_SUBDOMAIN, AnchorType::AnchorType_BOUNDARY, "",
       ParseResult::ERROR_RULE_NOT_MEANINGFUL},
      {"|*|||", UrlPatternType::UrlPatternType_WILDCARDED,
       AnchorType::AnchorType_BOUNDARY, AnchorType::AnchorType_BOUNDARY, "*||",
       ParseResult::SUCCESS},
      {"|xyz|", UrlPatternType::UrlPatternType_SUBSTRING,
       AnchorType::AnchorType_BOUNDARY, AnchorType::AnchorType_BOUNDARY, "xyz",
       ParseResult::SUCCESS},
      {"||x^yz", UrlPatternType::UrlPatternType_WILDCARDED,
       AnchorType::AnchorType_SUBDOMAIN, AnchorType::AnchorType_NONE, "x^yz",
       ParseResult::SUCCESS},
      {"||xyz|", UrlPatternType::UrlPatternType_SUBSTRING,
       AnchorType::AnchorType_SUBDOMAIN, AnchorType::AnchorType_BOUNDARY, "xyz",
       ParseResult::SUCCESS},
      {"x*y|z", UrlPatternType::UrlPatternType_WILDCARDED,
       AnchorType::AnchorType_NONE, AnchorType::AnchorType_NONE, "x*y|z",
       ParseResult::SUCCESS},
      {"**^", UrlPatternType::UrlPatternType_WILDCARDED,
       AnchorType::AnchorType_NONE, AnchorType::AnchorType_NONE, "**^",
       ParseResult::SUCCESS},
      {"||google.com", UrlPatternType::UrlPatternType_SUBSTRING,
       AnchorType::AnchorType_SUBDOMAIN, AnchorType::AnchorType_NONE,
       "google.com", ParseResult::SUCCESS},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<ParsedRule> rule =
        base::MakeUnique<ParsedRule>(CreateGenericParsedRule());
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
  std::unique_ptr<ParsedRule> rule =
      base::MakeUnique<ParsedRule>(CreateGenericParsedRule());
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
  } cases[] = {
      {base::MakeUnique<std::string>(""),
       ParseResult::ERROR_EMPTY_REDIRECT_URL},
      {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_URL},
      {base::MakeUnique<std::string>("google.com"), ParseResult::SUCCESS},
  };

  for (size_t i = 0; i < arraysize(cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<ParsedRule> rule =
        base::MakeUnique<ParsedRule>(CreateGenericParsedRule());
    // Copy the priority for later use.
    std::unique_ptr<std::string> redirect_url_copy =
        cases[i].redirect_url
            ? base::MakeUnique<std::string>(*cases[i].redirect_url)
            : nullptr;
    rule->action.redirect_url = std::move(cases[i].redirect_url);
    rule->action.type = api::declarative_net_request::RULE_ACTION_TYPE_REDIRECT;
    rule->priority = base::MakeUnique<int>(kMinValidPriority);
    IndexedRule indexed_rule;
    ParseResult result = CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(indexed_rule.redirect_url, *redirect_url_copy);
  }
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace  extensions
