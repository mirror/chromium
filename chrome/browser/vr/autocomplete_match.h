// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_AUTOCOMPLETE_MATCH_H_
#define CHROME_BROWSER_VR_AUTOCOMPLETE_MATCH_H_

#include <memory>

#include "base/macros.h"
#include "base/values.h"
#include "components/omnibox/browser/autocomplete_match.h"

class Profile;

namespace vr {

class BrowserUiInterface;

struct Suggestion {
  Suggestion(const base::string16& new_content,
             const base::string16& new_description,
             AutocompleteMatch::Type new_type);
  // Suggestion(const Suggestion& other);

  base::string16 content;
  base::string16 description;
  AutocompleteMatch::Type type;
};

struct VrAutocompleteResult {
  VrAutocompleteResult();
  ~VrAutocompleteResult();

  std::vector<Suggestion> matches;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_AUTOCOMPLETE_MATCH_H_
