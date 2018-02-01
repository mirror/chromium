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

  void SetAutocompletion(const Autocompletion& autocompletion);

 private:
  void OnUpdateInput(const TextInputInfo& info,
                     const TextInputInfo& previous_info) override;

  base::RepeatingCallback<void(const AutocompleteRequest&)>
      autocomplete_callback_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxTextField);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_OMNIBOX_TEXT_FIELD_H_
