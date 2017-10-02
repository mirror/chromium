// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"

class TabSearchProvider : public AutocompleteProvider, TabCallback {
 public:
  TabSearchProvider(AutocompleteProviderClient* provider_client);
  void Start(const AutocompleteInput& input, bool minimal_changes) override;
  void SearchBrowsers(const AutocompleteInput& input);
  bool TabMatches(bool incognito,
                  const GURL& url,
                  const base::string16& title) override;

 private:
  ~TabSearchProvider() override;

  AutocompleteProviderClient* provider_client_;
  base::string16 input_text_;
};
