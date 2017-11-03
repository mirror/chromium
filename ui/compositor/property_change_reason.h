// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_PROPERTY_CHANGE_REASON_H_
#define UI_COMPOSITOR_PROPERTY_CHANGE_REASON_H_

namespace ui {

enum class PropertyChangeReason {
  // The property value is set without an animation.
  SET,
  // The property value is being animated.
  ANIMATION,
};

}  // namespace ui

#endif  // UI_COMPOSITOR_PROPERTY_CHANGE_REASON_H_
