// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/browser/language_histogram.h"

#include <algorithm>
#include <map>
#include <set>

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace language {

namespace {

const char kLanguageHistogramCounters[] = "language_model_counters";

const int kMaxCountersSum = 1000;
const int kMinCountersSum = 10;
const float kCutoffRatio = 0.005f;
const float kDiscountFactor = 0.75f;

// Gets the sum of the counter for all languages in the histogram.
int GetCountersSum(const base::DictionaryValue& dict) {
  int sum = 0;
  int counter_value = 0;
  for (base::DictionaryValue::Iterator itr(dict); !itr.IsAtEnd();
       itr.Advance()) {
    if (itr.value().GetAsInteger(&counter_value))
      sum += counter_value;
  }
  return sum;
}

// Removes languages with small counter values and discount remaining counters.
void DiscountAndCleanCounters(base::DictionaryValue* dict) {
  std::set<std::string> remove_keys;

  int counter_value = 0;
  for (base::DictionaryValue::Iterator itr(*dict); !itr.IsAtEnd();
       itr.Advance()) {
    // Remove languages with invalid or small values.
    if (!itr.value().GetAsInteger(&counter_value) ||
        counter_value < (kCutoffRatio * kMaxCountersSum)) {
      remove_keys.insert(itr.key());
      continue;
    }

    // Discount the value.
    dict->SetInteger(itr.key(), counter_value * kDiscountFactor);
  }

  for (const std::string& lang_to_remove : remove_keys)
    dict->Remove(lang_to_remove, nullptr);
}

// Transforms the counters from prefs into a list of LanguageInfo structs.
std::vector<LanguageHistogram::LanguageInfo> GetAllLanguages(
    const base::DictionaryValue& dict) {
  int counters_sum = GetCountersSum(dict);

  // If the sample is not large enough yet, pretend there are no top languages.
  if (counters_sum < kMinCountersSum)
    return std::vector<LanguageHistogram::LanguageInfo>();

  std::vector<LanguageHistogram::LanguageInfo> top_languages;
  int counter_value = 0;
  for (base::DictionaryValue::Iterator itr(dict); !itr.IsAtEnd();
       itr.Advance()) {
    if (!itr.value().GetAsInteger(&counter_value))
      continue;
    top_languages.emplace_back(
        itr.key(), static_cast<float>(counter_value) / counters_sum);
  }
  return top_languages;
}

}  // namespace

LanguageHistogram::LanguageHistogram(PrefService* pref_service)
    : pref_service_(pref_service) {}

LanguageHistogram::~LanguageHistogram() = default;

// static
void LanguageHistogram::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kLanguageHistogramCounters);
}

std::vector<LanguageHistogram::LanguageInfo>
LanguageHistogram::GetTopLanguages() const {
  std::vector<LanguageHistogram::LanguageInfo> top_languages = GetAllLanguages(
      *pref_service_->GetDictionary(kLanguageHistogramCounters));

  std::sort(
      top_languages.begin(), top_languages.end(),
      [](LanguageHistogram::LanguageInfo a, LanguageHistogram::LanguageInfo b) {
        return a.frequency > b.frequency;
      });

  return top_languages;
}

float LanguageHistogram::GetLanguageFrequency(
    const std::string& language_code) const {
  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(kLanguageHistogramCounters);
  int counters_sum = GetCountersSum(*dict);
  // If the sample is not large enough yet, pretend there are no top languages.
  if (counters_sum < kMinCountersSum)
    return 0;

  int counter_value = 0;
  // If the key |language_code| does not exist, |counter_value| stays 0.
  dict->GetInteger(language_code, &counter_value);

  return static_cast<float>(counter_value) / counters_sum;
}

void LanguageHistogram::OnPageVisited(const std::string& language_code) {
  DictionaryPrefUpdate update(pref_service_, kLanguageHistogramCounters);
  base::DictionaryValue* dict = update.Get();
  int counter_value = 0;
  // If the key |language_code| does not exist, |counter_value| stays 0.
  dict->GetInteger(language_code, &counter_value);
  dict->SetInteger(language_code, counter_value + 1);

  if (GetCountersSum(*dict) > kMaxCountersSum)
    DiscountAndCleanCounters(dict);
}

void LanguageHistogram::ClearHistory(base::Time begin, base::Time end) {
  // Ignore all partial removals and react only to "entire" history removal.
  bool is_entire_history = (begin == base::Time() && end == base::Time::Max());
  if (!is_entire_history) {
    return;
  }

  pref_service_->ClearPref(kLanguageHistogramCounters);
}

}  // namespace language
