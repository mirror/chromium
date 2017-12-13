// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spellcheck_language_policy_handler.h"

#include "base/memory/ptr_util.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/spellcheck/common/spellcheck_common.h"

SpellcheckLanguagePolicyHandler::SpellcheckLanguagePolicyHandler()
    : TypeCheckingPolicyHandler(policy::key::kSpellcheckLanguage,
                                base::Value::Type::STRING) {}

SpellcheckLanguagePolicyHandler::~SpellcheckLanguagePolicyHandler() = default;

bool SpellcheckLanguagePolicyHandler::CheckPolicySettings(
    const policy::PolicyMap& policies,
    policy::PolicyErrorMap* errors) {
  const base::Value* value = nullptr;
  if (!CheckAndGetValue(policies, errors, &value))
    return false;
  return true;
}

void SpellcheckLanguagePolicyHandler::ApplyPolicySettings(
    const policy::PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value = policies.GetValue(policy_name());
  if (!value)
    return;

  const std::string string_value = value->GetString();

  std::vector<std::string> languages = base::SplitString(
      string_value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::unique_ptr<base::ListValue> forced_language_list =
      base::MakeUnique<base::ListValue>();
  for (const auto& language : languages) {
    std::string current_language =
        spellcheck::GetCorrespondingSpellCheckLanguage(language);
    if (!current_language.empty()) {
      forced_language_list->GetList().push_back(base::Value(current_language));
    } else {
      LOG(WARNING) << "Unknown language requested: \"" << language << "\"";
    }
  }

  prefs->SetValue(spellcheck::prefs::kEnableSpellcheck,
                  base::MakeUnique<base::Value>(true));
  prefs->SetValue(spellcheck::prefs::kSpellCheckForcedDictionaries,
                  std::move(forced_language_list));
}
