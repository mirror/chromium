// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_REMOTE_SUGGESTIONS_HELPER_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_REMOTE_SUGGESTIONS_HELPER_H_

#include "base/time/time.h"
#include "base/values.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/remote/remote_suggestion.h"

namespace ntp_snippets {

class RemoteSuggestionsHelper {
 public:
  struct FetchedCategory {
    Category category;
    CategoryInfo info;
    RemoteSuggestion::PtrVector suggestions;

    FetchedCategory(Category c, CategoryInfo&& info);
    FetchedCategory(FetchedCategory&&);             // = default, in .cc
    ~FetchedCategory();                             // = default, in .cc
    FetchedCategory& operator=(FetchedCategory&&);  // = default, in .cc
  };

  using FetchedCategoriesVector = std::vector<FetchedCategory>;

  static bool JsonToSnippets(const base::Value& parsed,
                             FetchedCategoriesVector* categories,
                             const base::Time& fetch_time);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_REMOTE_SUGGESTIONS_HELPER_H_
