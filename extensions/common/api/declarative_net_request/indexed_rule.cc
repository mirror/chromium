// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/indexed_rule.h"

#include <algorithm>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/api/declarative_net_request/constants.h"

namespace extensions {
namespace declarative_net_request {

namespace {

namespace flat_rule = ::url_pattern_index::flat;
namespace dnr_api = ::extensions::api::declarative_net_request;

// Helper class to parse the url filter of a Declarative Net Request API rule.
class UrlFilterParser {
 public:
  UrlFilterParser(std::string url_filter, IndexedRule* indexed_rule)
      : url_filter_(std::move(url_filter)),
        url_filter_len_(url_filter_.length()),
        index_(0),
        indexed_rule_(indexed_rule) {}

  void Parse() {
    DCHECK_EQ(0u, index_);

    ParseLeftAnchor();
    DCHECK_LE(index_, 2u);

    ParseFilterString();
    DCHECK(index_ == url_filter_len_ || index_ + 1 == url_filter_len_);

    ParseRightAnchor();
    DCHECK_EQ(url_filter_len_, index_);
  }

 private:
  void ParseLeftAnchor() {
    DCHECK_EQ(0u, index_);

    indexed_rule_->anchor_left = flat_rule::AnchorType_NONE;

    if (IsAtAnchor()) {
      index_++;
      indexed_rule_->anchor_left = flat_rule::AnchorType_BOUNDARY;
      if (IsAtAnchor()) {
        index_++;
        indexed_rule_->anchor_left = flat_rule::AnchorType_SUBDOMAIN;
      }
    }
  }

  void ParseFilterString() {
    indexed_rule_->url_pattern_type = flat_rule::UrlPatternType_SUBSTRING;
    size_t left_index = index_;
    while (index_ < url_filter_len_) {
      if (IsAtRightAnchor())
        break;
      if (IsAtSeparatorOrWildcard())
        indexed_rule_->url_pattern_type = flat_rule::UrlPatternType_WILDCARDED;
      index_++;
    }
    // Note: empty url patterns need to be supported.
    indexed_rule_->url_pattern =
        url_filter_.substr(left_index, index_ - left_index);
  }

  void ParseRightAnchor() {
    indexed_rule_->anchor_right = flat_rule::AnchorType_NONE;
    if (IsAtRightAnchor()) {
      index_++;
      indexed_rule_->anchor_right = flat_rule::AnchorType_BOUNDARY;
    }
  }

  bool IsAtSeparatorOrWildcard() const {
    return IsAtValidIndex() && (url_filter_[index_] == kSeparatorCharacter ||
                                url_filter_[index_] == kWildcardCharacter);
  }

  bool IsAtRightAnchor() const {
    return IsAtAnchor() && index_ > 0 && index_ + 1 == url_filter_len_;
  }

  bool IsAtValidIndex() const { return index_ < url_filter_len_; }

  bool IsAtAnchor() const {
    return IsAtValidIndex() && url_filter_[index_] == kAnchorCharacter;
  }

  static constexpr char kAnchorCharacter = '|';
  static constexpr char kSeparatorCharacter = '^';
  static constexpr char kWildcardCharacter = '*';

  const std::string url_filter_;
  const size_t url_filter_len_;
  size_t index_;
  IndexedRule* indexed_rule_;  // Must outlive this instance.
};

uint8_t GetOptionsMask(const dnr_api::Rule& parsed_rule) {
  uint8_t mask = flat_rule::OptionFlag_NONE;
  if (parsed_rule.action.type == dnr_api::RULE_ACTION_TYPE_WHITELIST)
    mask |= flat_rule::OptionFlag_IS_WHITELIST;
  if (parsed_rule.condition.url_filter_is_case_sensitive &&
      *parsed_rule.condition.url_filter_is_case_sensitive) {
    mask |= flat_rule::OptionFlag_IS_MATCH_CASE;
  }

  switch (parsed_rule.condition.domain_type) {
    case dnr_api::DOMAIN_TYPE_FIRST_PARTY:
      mask |= flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY;
      break;
    case dnr_api::DOMAIN_TYPE_THIRD_PARTY:
      mask |= flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY;
      break;
    case dnr_api::DOMAIN_TYPE_ANY:
    case dnr_api::DOMAIN_TYPE_NONE:
      mask |= (flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY |
               flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY);
      break;
  }
  return mask;
}

uint8_t GetActivationTypes(const dnr_api::Rule& parsed_rule) {
  // Extensions don't use any activation types currently.
  return flat_rule::ActivationType_NONE;
}

flat_rule::ElementType GetElementType(dnr_api::ResourceType resource_type) {
  switch (resource_type) {
    case dnr_api::RESOURCE_TYPE_NONE:
      return flat_rule::ElementType_NONE;
    case dnr_api::RESOURCE_TYPE_SUB_FRAME:
      return flat_rule::ElementType_SUBDOCUMENT;
    case dnr_api::RESOURCE_TYPE_STYLESHEET:
      return flat_rule::ElementType_STYLESHEET;
    case dnr_api::RESOURCE_TYPE_SCRIPT:
      return flat_rule::ElementType_SCRIPT;
    case dnr_api::RESOURCE_TYPE_IMAGE:
      return flat_rule::ElementType_IMAGE;
    case dnr_api::RESOURCE_TYPE_FONT:
      return flat_rule::ElementType_FONT;
    case dnr_api::RESOURCE_TYPE_OBJECT:
      return flat_rule::ElementType_OBJECT;
    case dnr_api::RESOURCE_TYPE_XMLHTTPREQUEST:
      return flat_rule::ElementType_XMLHTTPREQUEST;
    case dnr_api::RESOURCE_TYPE_PING:
      return flat_rule::ElementType_PING;
    case dnr_api::RESOURCE_TYPE_MEDIA:
      return flat_rule::ElementType_MEDIA;
    case dnr_api::RESOURCE_TYPE_WEBSOCKET:
      return flat_rule::ElementType_WEBSOCKET;
    case dnr_api::RESOURCE_TYPE_OTHER:
      return flat_rule::ElementType_OTHER;
  }
  NOTREACHED();
  return flat_rule::ElementType_NONE;
}

uint16_t GetResourceTypesMask(
    const std::vector<dnr_api::ResourceType>* resource_types) {
  uint16_t mask = flat_rule::ElementType_NONE;
  if (!resource_types)
    return mask;

  for (const auto resource_type : *resource_types)
    mask |= GetElementType(resource_type);
  return mask;
}

ParseResult ComputeElementTypes(const dnr_api::Rule& rule,
                                uint16_t* element_types) {
  uint16_t include_element_type_mask =
      GetResourceTypesMask(rule.condition.resource_types.get());
  uint16_t exclude_element_type_mask =
      GetResourceTypesMask(rule.condition.excluded_resource_types.get());
  if (include_element_type_mask & exclude_element_type_mask)
    return ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED;

  *element_types =
      include_element_type_mask
          ? include_element_type_mask
          : (flat_rule::ElementType_ANY & ~exclude_element_type_mask);
  return ParseResult::SUCCESS;
}

std::vector<std::string> GetSortedAndUniqueDomains(
    std::unique_ptr<std::vector<std::string>> domains) {
  if (!domains)
    return std::vector<std::string>();

  std::sort(domains->begin(), domains->end());
  domains->erase(std::unique(domains->begin(), domains->end()), domains->end());
  return *domains;
}

// See "declarative_net_request.idl" for what constitutes a meaningful rule.
bool IsMeaningfulRule(const IndexedRule& indexed_rule) {
  // TODO(karandeepb): This |element_types| condition should be modified once
  // "whitelist_recursive" action type is introduced.
  // Note: We check for equality to OBJECT_SUBREQUEST and not NONE, since
  // OBJECT_SUBREQUEST can never be part of the excluded resource types as it
  // isn't supported by Extensions.
  return indexed_rule.element_types !=
             flat_rule::ElementType_OBJECT_SUBREQUEST &&
         (!indexed_rule.url_pattern.empty() || !indexed_rule.domains.empty());
}

}  // namespace

IndexedRule::IndexedRule() = default;
IndexedRule::~IndexedRule() = default;

ParseResult CreateIndexedRule(std::unique_ptr<dnr_api::Rule> parsed_rule,
                              IndexedRule* indexed_rule) {
  DCHECK(indexed_rule);

  if (parsed_rule->id < kMinValidID)
    return ParseResult::ERROR_INVALID_RULE_ID;

  const bool is_redirect_rule =
      parsed_rule->action.type == dnr_api::RULE_ACTION_TYPE_REDIRECT;
  if (is_redirect_rule && (!parsed_rule->action.redirect_url ||
                           parsed_rule->action.redirect_url->empty()))
    return ParseResult::ERROR_EMPTY_REDIRECT_URL;
  if (is_redirect_rule && !parsed_rule->priority)
    return ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY;
  if (is_redirect_rule && *parsed_rule->priority < kMinValidPriority)
    return ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY;

  indexed_rule->id = base::checked_cast<uint32_t>(parsed_rule->id);
  indexed_rule->priority = base::checked_cast<uint32_t>(
      is_redirect_rule ? *parsed_rule->priority : kDefaultPriority);
  indexed_rule->options = GetOptionsMask(*parsed_rule);
  indexed_rule->activation_types = GetActivationTypes(*parsed_rule);

  {
    ParseResult result =
        ComputeElementTypes(*parsed_rule, &indexed_rule->element_types);
    if (result != ParseResult::SUCCESS)
      return result;
  }

  indexed_rule->domains =
      GetSortedAndUniqueDomains(std::move(parsed_rule->condition.domains));
  indexed_rule->excluded_domains = GetSortedAndUniqueDomains(
      std::move(parsed_rule->condition.excluded_domains));

  if (is_redirect_rule)
    indexed_rule->redirect_url = std::move(*parsed_rule->action.redirect_url);

  // Parse the |anchor_left|, |anchor_right|, |url_pattern_type| and
  // |url_pattern| fields.
  UrlFilterParser(parsed_rule->condition.url_filter
                      ? std::move(*parsed_rule->condition.url_filter)
                      : "",
                  indexed_rule)
      .Parse();

  if (!IsMeaningfulRule(*indexed_rule))
    return ParseResult::ERROR_RULE_NOT_MEANINGFUL;

  return ParseResult::SUCCESS;
}

}  // namespace declarative_net_request
}  // namespace extensions
