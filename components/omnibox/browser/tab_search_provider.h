// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "content/public/browser/web_contents.h"

class TabSearchProvider : public AutocompleteProvider {
 public:
  TabSearchProvider(AutocompleteProviderClient* provider_client);
  void Start(const AutocompleteInput& input, bool minimal_changes) override;
  void SearchBrowsers(const AutocompleteInput& input);
  bool FindTab(const AutocompleteInput& input, Browser* browser);
  bool TabMatches(const AutocompleteInput& input,
                  bool incognito,
                  content::WebContents* web_contents);

 private:
  ~TabSearchProvider() override;

  AutocompleteProviderClient* provider_client_;
};
