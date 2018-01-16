// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/omnibox_suggestions.h"

namespace vr {

OmniboxSuggestion::OmniboxSuggestion(
    const base::string16& new_content,
    const base::string16& new_description,
    const AutocompleteMatch::ACMatchClassifications&
        new_content_classifications,
    const AutocompleteMatch::ACMatchClassifications&
        new_description_classifications,
    AutocompleteMatch::Type new_type,
    GURL new_destination)
    : content(new_content),
      description(new_description),
      content_classifications(new_content_classifications),
      description_classifications(new_description_classifications),
      type(new_type),
      destination(new_destination) {}

OmniboxSuggestion::~OmniboxSuggestion() = default;

OmniboxSuggestion::OmniboxSuggestion(const OmniboxSuggestion& other) {
  content = other.content;
  content_classifications = other.content_classifications;
  description = other.description;
  description_classifications = other.description_classifications;
  type = other.type;
  destination = other.destination;
}

OmniboxSuggestions::OmniboxSuggestions() {}

OmniboxSuggestions::~OmniboxSuggestions() {}

}  // namespace vr
