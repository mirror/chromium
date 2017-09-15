// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"

class OmniboxResultView;

// FIXME: really questioning whether this should be an ImageButton
class OmniboxKeywordSearchButton : public views::ImageButton,
                                   views::ButtonListener {
 public:
  OmniboxKeywordSearchButton(OmniboxResultView* result_view)
      : ImageButton(this), result_view_(result_view) {
    // TODO: SetTooltipText(text);
    SetImageAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
  }

  // FIXME: comments
  // FIXME: is there button machinery I should be using here instead?
  void SetPressed();
  void ClearState();

  // views::View
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // FIXME: double-check that these are in the correct order
  // bool OnMouseDragged(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  OmniboxResultView* result_view_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxKeywordSearchButton);
};
