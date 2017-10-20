// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/autocomplete_match.h"

namespace vr {

VrAutocompleteResult::VrAutocompleteResult() {}

VrAutocompleteResult::~VrAutocompleteResult() {}

Suggestion::Suggestion(const base::string16& new_content,
                       const base::string16& new_description,
                       AutocompleteMatch::Type new_type)
    : content(new_content), description(new_description), type(new_type) {}

}  // namespace vr
