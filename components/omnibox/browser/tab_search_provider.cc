// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/tab_search_provider.h"

#include "base/i18n/break_iterator.h"
#include "base/strings/utf_string_conversions.h"

TabSearchProvider::TabSearchProvider(
    AutocompleteProviderClient* provider_client)
    : AutocompleteProvider(AutocompleteProvider::TYPE_TAB_SEARCH),
      provider_client_(provider_client) {}

TabSearchProvider::~TabSearchProvider() {}

void TabSearchProvider::Start(const AutocompleteInput& input,
                              bool minimal_changes) {
  matches_.clear();

  // Returning matches on NTP triggers dcheck. This is how other providers
  // avoid it.
  if (input.text().empty() || input.from_omnibox_focus())
    return;

  // search URL or also title?
  SearchBrowsers(input);
}

void TabSearchProvider::SearchBrowsers(const AutocompleteInput& input) {
  input_text_ = input.text();
  // Calls |TabMatches()| below with each tab data.
  provider_client_->IterateBrowserTabs(this);
}

bool TabSearchProvider::TabMatches(bool incognito,
                                   const GURL& url,
                                   const base::string16& title) {
  if (matches_.size() >= 1)
    // We have enough.
    return true;

  if (incognito != provider_client_->IsOffTheRecord())
    return false;

  // TODO: We'd like to skip looking in the active tab, but AFAICT
  // that requires reaching all the way up to the location bar.
  // (OmniboxEditController is close but subclass of correct one.)
  // (btw, you could use omnibox_client->IsNewTabPage(url) here)
  if (url == "chrome://newtab/")
    return false;

  base::i18n::BreakIterator word_iter(input_text_,
                                      base::i18n::BreakIterator::BREAK_WORD);
  if (!word_iter.Init())
    return false;
  // TODO: Remove the crap after the filename.
  base::string16 url_to_search = base::UTF8ToUTF16(url.spec());
  base::string16::size_type amount_searched = 0;
  std::vector<std::pair<base::string16::size_type, base::string16::size_type>>
      match_positions;
  // For each word in 'input', make sure it is in URL in order.
  // TODO: Should we search the title too? Might be better known to user.
  // Displaying bolded results in drop-down will be trickier.
  while (word_iter.Advance()) {
    if (word_iter.IsWord()) {
      base::string16::size_type offset =
          url_to_search.find(word_iter.GetString());
      // If a word not present, bail.
      if (offset == base::string16::npos)
        return false;
      // Handle if back-to-back with previous match, just extend previous.
      if (match_positions.empty() ||
          match_positions.back().first + match_positions.back().second <
              amount_searched + offset) {
        match_positions.push_back(std::make_pair(amount_searched + offset,
                                                 word_iter.GetString().size()));
      } else {
        match_positions.back().second += word_iter.GetString().size();
      }
      url_to_search =
          url_to_search.substr(offset + word_iter.GetString().size());
      amount_searched += offset + word_iter.GetString().size();
    }
  }
  if (match_positions.empty())
    return false;

  // Construct match using offsets.
  // TODO: Factor into subroutine.
  // TODO: Decide what score to use.
  AutocompleteMatch match(this, 1500, false, AutocompleteMatchType::TAB_SEARCH);
  // TODO: Doublecheck we are behaving correctly with respect to
  // inline autocomplete.
  match.allowed_to_be_default_match = true;
  match.destination_url = url;
  match.contents = base::UTF8ToUTF16(url.spec());
  for (auto i = match_positions.begin(); i != match_positions.end(); ++i) {
    if (i == match_positions.begin() && i->first != 0)
      match.contents_class.push_back(
          ACMatchClassification(0, ACMatchClassification::MATCH));
    match.contents_class.push_back(
        ACMatchClassification(i->first, ACMatchClassification::NONE));
    if (i->first + i->second < match.contents.size())
      match.contents_class.push_back(ACMatchClassification(
          i->first + i->second, ACMatchClassification::MATCH));
  }
  match.fill_into_edit = match.description =
      base::UTF8ToUTF16("Switch to tab '") + title + base::UTF8ToUTF16("'");
  match.description_class.push_back(
      ACMatchClassification(0, ACMatchClassification::MATCH));
  // TODO: Make a constant.
  match.RecordAdditionalInfo("incognito", incognito);
  matches_.push_back(match);
  return true;
}
