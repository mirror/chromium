// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/confirm_bubble_model.h"

#include "base/logging.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/ui/views/confirm_bubble_view.h"
#include "ui/gfx/rect.h"
#include "views/widget/widget.h"
#endif

ConfirmBubbleModel::ConfirmBubbleModel() {
}

ConfirmBubbleModel::~ConfirmBubbleModel() {
}

int ConfirmBubbleModel::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

string16 ConfirmBubbleModel::GetButtonLabel(BubbleButton button) const {
  return l10n_util::GetStringUTF16((button == BUTTON_OK) ? IDS_OK : IDS_CANCEL);
}

void ConfirmBubbleModel::Accept() {
}

void ConfirmBubbleModel::Cancel() {
}

string16 ConfirmBubbleModel::GetLinkText() const {
  return string16();
}

void ConfirmBubbleModel::LinkClicked() {
}

void ConfirmBubbleModel::Show(gfx::NativeView view,
                              const gfx::Point& origin,
                              ConfirmBubbleModel* model) {
#if defined(TOOLKIT_VIEWS)
  views::Widget* parent = views::Widget::GetTopLevelWidgetForNativeView(view);
  if (!parent)
    return;

  ConfirmBubbleView* bubble_view = new ConfirmBubbleView(model);
  Bubble* bubble = Bubble::Show(parent, gfx::Rect(origin, gfx::Size()),
                                views::BubbleBorder::NONE,
                                bubble_view, bubble_view);
  bubble->SizeToContents();
#else
  NOTIMPLEMENTED();  // Bug 99130: Implement it.
#endif
}

