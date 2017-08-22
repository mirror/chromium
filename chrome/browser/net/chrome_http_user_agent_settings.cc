// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_http_user_agent_settings.h"

#include "base/feature_list.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_util.h"

ChromeHttpUserAgentSettings::ChromeHttpUserAgentSettings(PrefService* prefs) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_accept_language_.Init(prefs::kAcceptLanguages, prefs);
  last_pref_accept_language_ = *pref_accept_language_;

  if (base::FeatureList::IsEnabled(features::kOverrideAcceptLanguageHeader)) {
    last_http_accept_language_ = net::HttpUtil::GenerateAcceptLanguageHeader(
        ExpandLanguageList(last_pref_accept_language_));
  } else {
    last_http_accept_language_ =
        net::HttpUtil::GenerateAcceptLanguageHeader(last_pref_accept_language_);
  }

  pref_accept_language_.MoveToThread(
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::IO));
}

ChromeHttpUserAgentSettings::~ChromeHttpUserAgentSettings() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

std::string ChromeHttpUserAgentSettings::ExpandLanguageList(
    const std::string& language_prefs) {
  const std::vector<std::string> languages = base::SplitString(
      language_prefs, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (languages.empty())
    return "";

  // We need to check for duplicates during the expansion.
  // We check for duplicates in the entire list, even though preferences should
  // not contain duplicates.
  std::unordered_set<std::string> seen(languages.begin(), languages.end());

  // TODO(claudiomagni): Handle "zh" separately.

  std::string final_str = "";
  size_t i = 0;
  while (i < languages.size()) {
    if (i == 0) {
      base::StringAppendF(&final_str, "%s", languages[i].c_str());
    } else {
      base::StringAppendF(&final_str, ",%s", languages[i].c_str());
    }

    // Extract the base language
    const std::vector<std::string> curr_tokens = base::SplitString(
        languages[i], "-", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (curr_tokens.size() == 2 && seen.find(curr_tokens[0]) == seen.end()) {
      const std::string& base_language = curr_tokens[0];

      // Look ahead and add the base language if the next language is not part
      // of the same family.
      const size_t j = i + 1;
      bool add = false;
      if (j >= languages.size()) {
        add = true;
      } else {
        const std::vector<std::string> next_tokens = base::SplitString(
            languages[j], "-", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
        if (next_tokens[0] != base_language) {
          add = true;
        }
      }

      if (add) {
        base::StringAppendF(&final_str, ",%s", base_language.c_str());
        seen.insert(base_language);
      }
    }

    ++i;
  }

  return final_str;
}

void ChromeHttpUserAgentSettings::CleanupOnUIThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_accept_language_.Destroy();
}

std::string ChromeHttpUserAgentSettings::GetAcceptLanguage() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::string new_pref_accept_language = *pref_accept_language_;

  if (new_pref_accept_language != last_pref_accept_language_) {
    if (base::FeatureList::IsEnabled(features::kOverrideAcceptLanguageHeader)) {
      last_http_accept_language_ = net::HttpUtil::GenerateAcceptLanguageHeader(
          ExpandLanguageList(new_pref_accept_language));
    } else {
      last_http_accept_language_ =
          net::HttpUtil::GenerateAcceptLanguageHeader(new_pref_accept_language);
    }
    last_pref_accept_language_ = new_pref_accept_language;
  }

  return last_http_accept_language_;
}

std::string ChromeHttpUserAgentSettings::GetUserAgent() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  return ::GetUserAgent();
}

