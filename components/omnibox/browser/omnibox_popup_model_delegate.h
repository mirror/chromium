// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_POPUP_MODEL_DELEGATE_H_
#define COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_POPUP_MODEL_DELEGATE_H_

class OmniboxPopupModelDelegate {
 public:
  // Called when any relevant data changes.  This rolls together several
  // separate pieces of data into one call so we can update all the UI
  // efficiently:
  //   |text| is either the new temporary text from the user manually selecting
  //     a different match, or the inline autocomplete text.  We distinguish by
  //     checking if |destination_for_temporary_text_change| is NULL.
  //   |destination_for_temporary_text_change| is NULL (if temporary text should
  //     not change) or the pre-change destination URL (if temporary text should
  //     change) so we can save it off to restore later.
  //   |keyword| is the keyword to show a hint for if |is_keyword_hint| is true,
  //     or the currently selected keyword if |is_keyword_hint| is false (see
  //     comments on keyword_ and is_keyword_hint_).
  virtual void OnPopupDataChanged(const base::string16& text,
                                  GURL* destination_for_temporary_text_change,
                                  const base::string16& keyword,
                                  bool is_keyword_hint) = 0;
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_POPUP_MODEL_DELEGATE_H_
