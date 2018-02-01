// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_OMNIBOX_TEXT_FIELD_H_
#define CHROME_BROWSER_VR_ELEMENTS_OMNIBOX_TEXT_FIELD_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/vr/elements/text_input.h"
#include "chrome/browser/vr/model/text_input_info.h"
#include "chrome/browser/vr/text_input_delegate.h"

#include "chrome/browser/vr/model/omnibox_suggestions.h"

namespace vr {

class OmniboxTextField : public TextInput {
 public:
  OmniboxTextField(float font_height_meters,
                   OnFocusChangedCallback focus_changed_callback,
                   OnInputEditedCallback input_edit_callback,
                   base::RepeatingCallback<void(const AutocompleteRequest&)>
                       autocomplete_callback);
  ~OmniboxTextField() override;

  void OnInputEdited(const TextInputInfo& info) override;
  void OnInputCommitted(const TextInputInfo& info) override;

  void SetTopMatch(const OmniboxSuggestion& match);
  void ClearTopMatch();

 private:
  TextInputInfo ProcessInput(const TextInputInfo& info) override;

  // Could we instead get this through ProcessInput?  That method could take in
  // the old and new input.
  TextInputInfo previous_input_;

  std::unique_ptr<OmniboxSuggestion> top_match_;

  base::RepeatingCallback<void(const AutocompleteRequest&)>
      autocomplete_callback_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxTextField);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_OMNIBOX_TEXT_FIELD_H_
