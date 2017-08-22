// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/test_utils.h"

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "extensions/browser/api/declarative_net_request/constants.h"
#include "extensions/common/value_builder.h"

namespace extensions {
namespace declarative_net_request {

namespace {

// Returns a ListValue corresponding to a vector of strings.
std::unique_ptr<base::ListValue> GetListValue(
    const std::vector<std::string>& vec) {
  ListBuilder builder;
  for (const std::string& str : vec)
    builder.Append(str);
  return builder.Build();
}

}  // namespace

TestRuleCondition::TestRuleCondition() = default;
TestRuleCondition::~TestRuleCondition() = default;
TestRuleCondition::TestRuleCondition(const TestRuleCondition&) = default;
TestRuleCondition& TestRuleCondition::operator=(const TestRuleCondition&) =
    default;

std::unique_ptr<base::DictionaryValue> TestRuleCondition::ToValue() const {
  std::unique_ptr<base::DictionaryValue> value =
      base::MakeUnique<base::DictionaryValue>();
  if (url_filter)
    value->SetString(kUrlFilterKey, *url_filter);
  if (is_url_filter_case_sensitive) {
    value->SetBoolean(kIsUrlFilterCaseSensitiveKey,
                      *is_url_filter_case_sensitive);
  }
  if (domains)
    value->Set(kDomainsKey, GetListValue(*domains));
  if (excluded_domains)
    value->Set(kExcludedDomainsKey, GetListValue(*excluded_domains));
  if (resource_types)
    value->Set(kResourceTypesKey, GetListValue(*resource_types));
  if (excluded_resource_types)
    value->Set(kExcludedResourceTypesKey,
               GetListValue(*excluded_resource_types));
  if (domain_type)
    value->SetString(kDomainTypeKey, *domain_type);
  return value;
}

TestRuleAction::TestRuleAction() = default;
TestRuleAction::~TestRuleAction() = default;
TestRuleAction::TestRuleAction(const TestRuleAction&) = default;
TestRuleAction& TestRuleAction::operator=(const TestRuleAction&) = default;

std::unique_ptr<base::DictionaryValue> TestRuleAction::ToValue() const {
  std::unique_ptr<base::DictionaryValue> value =
      base::MakeUnique<base::DictionaryValue>();
  if (type)
    value->SetString(kRuleActionTypeKey, *type);
  if (redirect_url)
    value->SetString(kRedirectUrlKey, *redirect_url);
  return value;
}

TestRule::TestRule() = default;
TestRule::~TestRule() = default;
TestRule::TestRule(const TestRule&) = default;
TestRule& TestRule::operator=(const TestRule&) = default;

std::unique_ptr<base::DictionaryValue> TestRule::ToValue() const {
  std::unique_ptr<base::DictionaryValue> value =
      base::MakeUnique<base::DictionaryValue>();
  if (id)
    value->SetInteger(kIDKey, *id);
  if (priority)
    value->SetInteger(kPriorityKey, *priority);
  if (condition)
    value->Set(kRuleConditionKey, condition->ToValue());
  if (action)
    value->Set(kRuleActionKey, action->ToValue());
  return value;
}

TestRule GetGenericRule() {
  TestRuleCondition condition;
  condition.url_filter = std::string("filter");
  TestRuleAction action;
  action.type = std::string("blacklist");
  TestRule rule;
  rule.id = 1;
  rule.action = action;
  rule.condition = condition;
  return rule;
}

}  // namespace declarative_net_request
}  // namespace extensions
