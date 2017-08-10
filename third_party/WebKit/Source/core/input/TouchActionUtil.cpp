// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/input/TouchActionUtil.h"

#include "core/dom/Node.h"
#include "core/dom/NodeComputedStyle.h"
#include "core/frame/Settings.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutObject.h"

namespace blink {
namespace TouchActionUtil {

TouchAction ComputeEffectiveTouchAction(const Node& node) {
  TouchAction touch_action = TouchAction::kTouchActionAuto;
  if (node.GetComputedStyle()) {
    TouchAction touch_action =
        node.GetComputedStyle()->GetEffectiveTouchAction();
    LOG(ERROR) << node.GetDocument().GetSettings()->GetForceEnableZoom();
    if (touch_action != TouchAction::kTouchActionNone &&
        node.GetDocument().GetSettings()->GetForceEnableZoom())
      touch_action |= TouchAction::kTouchActionPinchZoom;
  }

  return touch_action;
}

}  // namespace TouchActionUtil
}  // namespace blink
