// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webshare/share_target_pref_helper.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/webshare/share_target.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

void UpdateShareTargetInPrefs(const GURL& manifest_url,
                              const content::Manifest& manifest,
                              PrefService* pref_service) {
  DictionaryPrefUpdate update(pref_service, prefs::kWebShareVisitedTargets);
  base::DictionaryValue* share_target_dict = update.Get();

  // Manifest does not contain a share_target field, or it does but there is no
  // url_template field.
  if (!manifest.share_target.has_value() ||
      manifest.share_target.value().url_template.is_null()) {
    share_target_dict->RemoveWithoutPathExpansion(manifest_url.spec(), nullptr);
    return;
  }

  std::string url_template =
      base::UTF16ToUTF8(manifest.share_target.value().url_template.string());

  constexpr char kNameKey[] = "name";
  constexpr char kUrlTemplateKey[] = "url_template";

  std::unique_ptr<base::DictionaryValue> origin_dict(new base::DictionaryValue);

  if (!manifest.name.is_null()) {
    origin_dict->SetStringWithoutPathExpansion(kNameKey,
                                               manifest.name.string());
  }
  origin_dict->SetStringWithoutPathExpansion(kUrlTemplateKey,
                                             url_template);

  share_target_dict->SetWithoutPathExpansion(manifest_url.spec(),
                                             std::move(origin_dict));
}

std::vector<ShareTarget> GetShareTargetsInPrefs(PrefService* pref_service) {
  std::unique_ptr<base::DictionaryValue> share_targets_dict =
      pref_service->GetDictionary(prefs::kWebShareVisitedTargets)
          ->CreateDeepCopy();

  std::vector<ShareTarget> share_targets;
  for (const auto& it : *share_targets_dict) {
    GURL manifest_url(it.first);

    const base::DictionaryValue* share_target_dict;
    bool result = it.second->GetAsDictionary(&share_target_dict);
    DCHECK(result);

    std::string name;
    share_target_dict->GetString("name", &name);

    std::string url_template;
    share_target_dict->GetString("url_template", &url_template);

    share_targets.emplace_back(std::move(manifest_url), std::move(name),
                               std::move(url_template));
  }

  return share_targets;
}
