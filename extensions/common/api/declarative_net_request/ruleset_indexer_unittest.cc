// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/ruleset_indexer.h"

#include <map>
#include <string>

#include "base/strings/stringprintf.h"
#include "base/test/histogram_tester.h"
#include "components/url_pattern_index/flat/url_pattern_index_generated.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/flat/extension_ruleset_generated.h"
#include "extensions/common/api/declarative_net_request/indexed_rule.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace flat_rule = ::url_pattern_index::flat;
using RulesetIndexerTest = ::testing::Test;

constexpr char kHistogramName[] =
    "Extensions.DeclarativeNetRequest.ManifestRulesCount";

// Helper to convert a flatbuffer string to a std::string.
std::string ToString(const flatbuffers::String* string) {
  DCHECK(string);
  return std::string(string->c_str(), string->size());
}

// Helper to convert a flatbuffer vector of strings to a std::vector.
std::vector<std::string> ToVector(
    const ::flatbuffers::Vector<::flatbuffers::Offset<::flatbuffers::String>>*
        vec) {
  if (!vec)
    return std::vector<std::string>();
  std::vector<std::string> result;
  for (auto* str : *vec)
    result.push_back(ToString(str));
  return result;
}

// Helper to create an IndexedRule.
IndexedRule CreateIndexedRule(uint32_t id,
                              uint32_t priority,
                              uint8_t options,
                              uint16_t element_types,
                              uint8_t activation_types,
                              flat_rule::UrlPatternType url_pattern_type,
                              flat_rule::AnchorType anchor_left,
                              flat_rule::AnchorType anchor_right,
                              std::string url_pattern,
                              std::vector<std::string> domains,
                              std::vector<std::string> excluded_domains,
                              std::string redirect_url) {
  IndexedRule rule;
  rule.id = id;
  rule.priority = priority;
  rule.options = options;
  rule.element_types = element_types;
  rule.activation_types = activation_types;
  rule.url_pattern_type = url_pattern_type;
  rule.anchor_left = anchor_left;
  rule.anchor_right = anchor_right;
  rule.url_pattern = std::move(url_pattern);
  rule.domains = std::move(domains);
  rule.excluded_domains = std::move(excluded_domains);
  rule.redirect_url = std::move(redirect_url);
  return rule;
}

// Compares |indexed_rule| and |rule| for equality.
bool AreRulesEqual(const IndexedRule* indexed_rule,
                   const flat_rule::UrlRule* rule) {
  bool equal = true;
  equal &= (indexed_rule->id == rule->id());
  equal &= (indexed_rule->priority == rule->priority());
  equal &= (indexed_rule->options == rule->options());
  equal &= (indexed_rule->element_types == rule->element_types());
  equal &= (indexed_rule->activation_types == rule->activation_types());
  equal &= (indexed_rule->url_pattern_type == rule->url_pattern_type());
  equal &= (indexed_rule->anchor_left == rule->anchor_left());
  equal &= (indexed_rule->anchor_right == rule->anchor_right());
  equal &= (indexed_rule->url_pattern == ToString(rule->url_pattern()));
  equal &= (indexed_rule->domains == ToVector(rule->domains_included()));
  equal &=
      (indexed_rule->excluded_domains == ToVector(rule->domains_excluded()));

  // Ignore redirect url since it's not stored as part of flat_rule::UrlRule.
  return equal;
}

// Returns all UrlRule(s) in the given |index|.
std::vector<const flat_rule::UrlRule*> GetAllRulesFromIndex(
    const flat_rule::UrlPatternIndex* index) {
  std::vector<const flat_rule::UrlRule*> result;

  // Iterate over all ngrams and add their corresponding rules.
  for (auto* ngram_to_rules : *index->ngram_index()) {
    if (ngram_to_rules == index->ngram_index_empty_slot())
      continue;
    for (const auto* rule : *ngram_to_rules->rule_list())
      result.push_back(rule);
  }

  // Add all fallback rules.
  for (const auto* rule : *index->fallback_rules())
    result.push_back(rule);

  return result;
}

// Verifies that both |rules| and |index| correspond to the same set of rules
// (in different representations).
void VerifyIndexEquality(const std::vector<IndexedRule>& rules,
                         const flat_rule::UrlPatternIndex* index) {
  struct RulePair {
    const IndexedRule* indexed_rule = nullptr;
    const flat_rule::UrlRule* url_rule = nullptr;
  };

  // Build a map from rule IDs to RulePair(s).
  std::map<uint32_t, RulePair> map;

  for (const auto& rule : rules) {
    EXPECT_FALSE(map[rule.id].indexed_rule);
    map[rule.id].indexed_rule = &rule;
  }

  std::vector<const flat_rule::UrlRule*> flat_rules =
      GetAllRulesFromIndex(index);
  for (const auto* rule : flat_rules) {
    EXPECT_FALSE(map[rule->id()].url_rule);
    map[rule->id()].url_rule = rule;
  }

  // Iterate over the map and verify equality of the two representations.
  for (const auto& elem : map) {
    EXPECT_TRUE(AreRulesEqual(elem.second.indexed_rule, elem.second.url_rule))
        << base::StringPrintf("Rule with id %u was incorrectly indexed",
                              elem.first);
  }
}

// Verifies that |extension_metadata| is sorted by ID and corresponds to rules
// in |redirect_rules|.
void VerifyExtensionMetadata(
    const std::vector<IndexedRule>& redirect_rules,
    const ::flatbuffers::Vector<
        flatbuffers::Offset<flat::ExtensionRuleMetadata>>* extension_metdata) {
  struct MetadataPair {
    const IndexedRule* indexed_rule = nullptr;
    const flat::ExtensionRuleMetadata* metadata = nullptr;
  };

  // Build a map from IDs to MetadataPair(s).
  std::map<uint32_t, MetadataPair> map;

  for (const auto& rule : redirect_rules) {
    EXPECT_FALSE(map[rule.id].indexed_rule);
    map[rule.id].indexed_rule = &rule;
  }

  int prev_id = kMinValidID - 1;
  for (const auto* metadata : *extension_metdata) {
    EXPECT_FALSE(map[metadata->id()].metadata);
    map[metadata->id()].metadata = metadata;

    // Also verify that the metadata vector is sorted by ID.
    int cur_id = static_cast<int>(metadata->id());
    EXPECT_LE(prev_id, cur_id) << "|extension_metdata| is not sorted by ID";
    prev_id = cur_id;
  }

  // Iterate over the map and verify equality of the redirect rules.
  for (const auto& elem : map) {
    EXPECT_EQ(elem.second.indexed_rule->redirect_url,
              ToString(elem.second.metadata->redirect_url()))
        << base::StringPrintf(
               "Redirect rule with id %u was incorrectly indexed", elem.first);
  }
}

// Helper which:
//    - Constructs an ExtensionIndexedRuleset flatbuffer from the passed
//      IndexedRule(s) using RulesetIndexer.
//    - Verifies that the ExtensionIndexedRuleset created is valid.
void AddRulesAndVerifyIndex(const std::vector<IndexedRule>& blacklist_rules,
                            const std::vector<IndexedRule>& whitelist_rules,
                            const std::vector<IndexedRule>& redirect_rules) {
  RulesetIndexer indexer;
  for (const auto& rule : blacklist_rules)
    indexer.AddUrlRule(rule);
  for (const auto& rule : whitelist_rules)
    indexer.AddUrlRule(rule);
  for (const auto& rule : redirect_rules)
    indexer.AddUrlRule(rule);

  RulesetIndexer::SerializedData data = indexer.FinishAndGetData();
  flatbuffers::Verifier verifier(data.first, data.second);
  ASSERT_TRUE(flat::VerifyExtensionIndexedRulesetBuffer(verifier));

  const flat::ExtensionIndexedRuleset* ruleset =
      flat::GetExtensionIndexedRuleset(data.first);
  ASSERT_TRUE(ruleset);

  VerifyIndexEquality(blacklist_rules, ruleset->blacklist_index());
  VerifyIndexEquality(whitelist_rules, ruleset->whitelist_index());
  VerifyIndexEquality(redirect_rules, ruleset->redirect_index());
  VerifyExtensionMetadata(redirect_rules, ruleset->extension_metdata());
}

TEST_F(RulesetIndexerTest, TestEmptyIndex) {
  base::HistogramTester tester;
  AddRulesAndVerifyIndex({}, {}, {});
  tester.ExpectUniqueSample(kHistogramName, 0 /*sample*/, 1 /*count*/);
}

TEST_F(RulesetIndexerTest, MultipleRules) {
  std::vector<IndexedRule> blacklist_rules;
  std::vector<IndexedRule> whitelist_rules;
  std::vector<IndexedRule> redirect_rules;
  base::HistogramTester tester;

  // Explicitly push the elements instead of using the initializer list
  // constructor, because it does not support move-only types.
  blacklist_rules.push_back(CreateIndexedRule(
      7, kMinValidPriority, flat_rule::OptionFlag_NONE,
      flat_rule::ElementType_OBJECT, flat_rule::ActivationType_NONE,
      flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_NONE,
      flat_rule::AnchorType_BOUNDARY, "google.com|", {"a.com"}, {"x.a.com"},
      ""));
  blacklist_rules.push_back(CreateIndexedRule(
      2, kMinValidPriority, flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY,
      flat_rule::ElementType_IMAGE | flat_rule::ElementType_WEBSOCKET,
      flat_rule::ActivationType_NONE, flat_rule::UrlPatternType_WILDCARDED,
      flat_rule::AnchorType_NONE, flat_rule::AnchorType_NONE, "*google*",
      {"a.com"}, {}, ""));

  redirect_rules.push_back(CreateIndexedRule(
      15, 2, flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY,
      flat_rule::ElementType_IMAGE, flat_rule::ActivationType_NONE,
      flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_SUBDOMAIN,
      flat_rule::AnchorType_BOUNDARY, "||google.com|", {}, {},
      "http://example1.com"));
  redirect_rules.push_back(CreateIndexedRule(
      10, 2, flat_rule::OptionFlag_NONE,
      flat_rule::ElementType_SUBDOCUMENT | flat_rule::ElementType_SCRIPT,
      flat_rule::ActivationType_NONE, flat_rule::UrlPatternType_SUBSTRING,
      flat_rule::AnchorType_NONE, flat_rule::AnchorType_NONE, "example1", {},
      {"a.com"}, "http://example2.com"));
  redirect_rules.push_back(CreateIndexedRule(
      9, 3, flat_rule::OptionFlag_NONE, flat_rule::ElementType_NONE,
      flat_rule::ActivationType_NONE, flat_rule::UrlPatternType_WILDCARDED,
      flat_rule::AnchorType_NONE, flat_rule::AnchorType_NONE, "*", {}, {},
      "http://example2.com"));

  whitelist_rules.push_back(CreateIndexedRule(
      17, kMinValidPriority, flat_rule::OptionFlag_IS_WHITELIST,
      flat_rule::ElementType_PING | flat_rule::ElementType_SCRIPT,
      flat_rule::ActivationType_NONE, flat_rule::UrlPatternType_SUBSTRING,
      flat_rule::AnchorType_SUBDOMAIN, flat_rule::AnchorType_NONE,
      "||example1.com", {"xyz.com"}, {}, ""));
  whitelist_rules.push_back(CreateIndexedRule(
      16, kMinValidPriority,
      flat_rule::OptionFlag_IS_WHITELIST | flat_rule::OptionFlag_IS_MATCH_CASE,
      flat_rule::ElementType_IMAGE, flat_rule::ActivationType_NONE,
      flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_NONE,
      flat_rule::AnchorType_NONE, "example3", {}, {}, ""));

  AddRulesAndVerifyIndex(blacklist_rules, whitelist_rules, redirect_rules);
  tester.ExpectUniqueSample(kHistogramName, 7 /*sample*/, 1 /*count*/);
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
