// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/bubble/bubble_window_targeter.h"

#include "ui/aura/window.h"
#include "ui/gfx/path.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/bubble/bubble_frame_view.h"

namespace views {

BubbleWindowTargeter::BubbleWindowTargeter(BubbleDialogDelegateView* bubble) {
  const gfx::Insets insets = bubble->GetBubbleFrameView()->GetInsets();
  SetInsets(insets, insets);
}

BubbleWindowTargeter::~BubbleWindowTargeter() {}

}  // namespace views
