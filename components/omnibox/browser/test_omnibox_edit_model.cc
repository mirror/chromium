// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/test_omnibox_edit_model.h"

#include "components/omnibox/browser/test_omnibox_client.h"

TestOmniboxEditModel::TestOmniboxEditModel(OmniboxView* view,
                                           OmniboxEditController* controller)
    : OmniboxEditModel(view,
                       controller,
                       std::make_unique<TestOmniboxClient>()) {}

bool TestOmniboxEditModel::PopupIsOpen() const {
  return popup_is_open_;
}

void TestOmniboxEditModel::OnUpOrDownKeyPressed(int count) {
  up_or_down_count_ = count;
}

AutocompleteMatch TestOmniboxEditModel::CurrentMatch(GURL*) const {
  return current_match_;
};

void TestOmniboxEditModel::SetCurrentMatch(const AutocompleteMatch& match) {
  current_match_ = match;
}