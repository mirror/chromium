// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_TEST_OMNIBOX_EDIT_MODEL_H_
#define COMPONENTS_OMNIBOX_BROWSER_TEST_OMNIBOX_EDIT_MODEL_H_

#include "base/macros.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_edit_model.h"

// Fake implementation of OmniboxEditModel for use in tests.
class TestOmniboxEditModel : public OmniboxEditModel {
 public:
  TestOmniboxEditModel(OmniboxView* view,
                       OmniboxEditController* controller,
                       std::unique_ptr<OmniboxClient> client);

  // OmniboxEditModel:
  bool PopupIsOpen() const override;
  void OnUpOrDownKeyPressed(int count) override;
  AutocompleteMatch CurrentMatch(GURL*) const override;

  void SetPopupIsOpen(bool open) { popup_is_open_ = open; }

  int up_or_down_count() const { return up_or_down_count_; }
  void set_up_or_down_count(int count) { up_or_down_count_ = count; }

  void SetCurrentMatch(const AutocompleteMatch& match);

 private:
  bool popup_is_open_;
  int up_or_down_count_;
  AutocompleteMatch current_match_;

  DISALLOW_COPY_AND_ASSIGN(TestOmniboxEditModel);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_TEST_OMNIBOX_EDIT_MODEL_H_
