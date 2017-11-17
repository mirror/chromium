// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_list_sorter.h"

#include <algorithm>
#include <tuple>

#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/password_ui_utils.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace password_manager {

namespace {

constexpr char kSortKeyPartsSeparator = ' ';

// The character that is added to a sort key if there is no federation.
// Note: to separate the entries w/ federation and the entries w/o federation,
// this character should be alphabetically smaller than real federations.
constexpr char kSortKeyNoFederationSymbol = '-';

// TODO(crbug.com/785926): The sort key contains password values (if present)
// for legacy reasons. An alternative way to construct the key is by omitting
// the password. Once the statistics prove it's low-risk, remove the LEGACY
// types of key.
enum class KeyType { LEGACY, PASSWORDLESS };

// Creates the sort key as described at the CreateSortKey definition comment,
// only with passwords included or not, depending on |key_type|.
std::string CreateSortKeyWithKeyType(const autofill::PasswordForm& form,
                                     PasswordEntryType entry_type,
                                     KeyType key_type) {
  std::string shown_origin;
  GURL link_url;
  std::tie(shown_origin, link_url) = GetShownOriginAndLinkUrl(form);

  const auto facet_uri =
      FacetURI::FromPotentiallyInvalidSpec(form.signon_realm);
  const bool is_android_uri = facet_uri.IsValidAndroidFacetURI();

  if (is_android_uri) {
    // In case of Android credentials |GetShownOriginAndLinkURl| might return
    // the app display name, e.g. the Play Store name of the given application.
    // This might or might not correspond to the eTLD+1, which is why
    // |shown_origin| is set to the reversed android package name in this case,
    // e.g. com.example.android => android.example.com.
    shown_origin = SplitByDotAndReverse(facet_uri.android_package_name());
  }

  std::string site_name =
      net::registry_controlled_domains::GetDomainAndRegistry(
          shown_origin,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (site_name.empty())  // e.g. localhost.
    site_name = shown_origin;

  std::string key = site_name + kSortKeyPartsSeparator;

  // Since multiple distinct credentials might have the same site name, more
  // information is added. For Android credentials this includes the full
  // canonical spec which is guaranteed to be unique for a given App.
  key += is_android_uri ? facet_uri.canonical_spec()
                        : SplitByDotAndReverse(shown_origin);

  if (entry_type == PasswordEntryType::SAVED) {
    key += kSortKeyPartsSeparator + base::UTF16ToUTF8(form.username_value);
    if (key_type == KeyType::LEGACY)
      key += kSortKeyPartsSeparator + base::UTF16ToUTF8(form.password_value);

    key += kSortKeyPartsSeparator;
    if (!form.federation_origin.unique())
      key += form.federation_origin.host();
    else
      key += kSortKeyNoFederationSymbol;
  }

  // To separate HTTP/HTTPS credentials, add the scheme to the key.
  return key += kSortKeyPartsSeparator + link_url.scheme();
}

// Sorts the |list| and creates |duplicates| as described in the declaration
// comment of SortEntriesAndHideDuplicates, only with passing |key_type| to
// CreateSortKeyWithKeyType.
void SortEntriesAndHideDuplicatesForKeyType(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* list,
    DuplicatesMap* duplicates,
    PasswordEntryType entry_type,
    KeyType key_type) {
  std::vector<std::pair<std::string, std::unique_ptr<autofill::PasswordForm>>>
      keys_to_forms;
  keys_to_forms.reserve(list->size());
  for (auto& form : *list) {
    std::string key = CreateSortKeyWithKeyType(*form, entry_type, key_type);
    keys_to_forms.push_back(std::make_pair(std::move(key), std::move(form)));
  }

  std::sort(
      keys_to_forms.begin(), keys_to_forms.end(),
      [](const std::pair<std::string, std::unique_ptr<autofill::PasswordForm>>&
             left,
         const std::pair<std::string, std::unique_ptr<autofill::PasswordForm>>&
             right) { return left.first < right.first; });

  list->clear();
  duplicates->clear();
  std::string previous_key;
  for (auto& key_to_form : keys_to_forms) {
    if (key_to_form.first != previous_key) {
      list->push_back(std::move(key_to_form.second));
      previous_key = key_to_form.first;
    } else {
      duplicates->insert(
          std::make_pair(previous_key, std::move(key_to_form.second)));
    }
  }
}

}  // namespace

std::string CreateSortKey(const autofill::PasswordForm& form,
                          PasswordEntryType entry_type) {
  return CreateSortKeyWithKeyType(form, entry_type, KeyType::LEGACY);
}

void SortEntriesAndHideDuplicates(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* list,
    DuplicatesMap* duplicates,
    PasswordEntryType entry_type) {
  size_t passwordless_duplicates = 0;
  if (entry_type == PasswordEntryType::SAVED) {
    // First compute the result for the passwordless key, for statistics.
    SortEntriesAndHideDuplicatesForKeyType(list, duplicates, entry_type,
                                           KeyType::PASSWORDLESS);
    passwordless_duplicates = duplicates->size();

    // Now return the duplicates back to the |list| to allow recomputing this
    // for the legacy key.
    for (auto& duplicate_iterator : *duplicates) {
      list->push_back(std::move(duplicate_iterator.second));
    }
  }

  SortEntriesAndHideDuplicatesForKeyType(list, duplicates, entry_type,
                                         KeyType::LEGACY);

  if (entry_type == PasswordEntryType::SAVED) {
    UMA_HISTOGRAM_COUNTS_100("PasswordManager.DeltaPasswordlessDuplicates",
                             passwordless_duplicates - duplicates->size());
  }
}

}  // namespace password_manager
