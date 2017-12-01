// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEW_IDS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEW_IDS_H_

#include "components/autofill/core/browser/field_types.h"

// This defines an enumeration of IDs that can uniquely identify a view within
// the scope of the local and upload credit card save bubbles.

namespace autofill {

enum class DialogViewId : int {
  VIEW_ID_NONE = 0,
  CONTENT_VIEW,        // The main content view
  DIALOG_CLIENT_VIEW,  // Contains the clickable buttons

  // The following are secondary views or ones contained by CONTENT_VIEW.
  DESCRIPTION_VIEW,  // Displays card icon, network, last four, and expiration
  REQUEST_CVC_VIEW,  // Secondary main content view for the CVC fix flow
  CVC_ENTRY_VIEW,    // Contains the CVC entry textfield and hint image
  FOOTNOTE_VIEW,     // Contains the legal messages for upload save

  // The following are views::LabelButton objects (clickable).
  OK_BUTTON,      // Can say [Save], [Next], and [Confirm] depend on context
  CANCEL_BUTTON,  // Typically says [No thanks]

  // The following are views::Link objects (clickable).
  LEARN_MORE_LINK,

  // The following are views::Label objects.
  EXPLANATION_LABEL,      // Explains the benefit of adding a card to Google
  CVC_EXPLANATION_LABEL,  // Explains that CVC is needed in order to save
  NETWORK_AND_LAST_FOUR_DIGITS_LABEL,
  LAST_FOUR_DIGITS_LABEL,
  EXPIRATION_DATE_LABEL,

  // The following are views::StyledLabel objects.
  LEGAL_MESSAGE_LINE_LABEL,  // The entire legal message in the footer

  // The following are views::ImageView objects.
  CARD_TYPE_ICON_IMAGE,  // Card type logo
  CVC_HINT_IMAGE,        // Icon that shows example CVC placement on card

  // The following are views::Textfield objects.
  CVC_TEXTFIELD,  // Used for CVC entry
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEW_IDS_H_
