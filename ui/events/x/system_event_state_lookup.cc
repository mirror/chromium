// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/x/system_event_state_lookup.h"

#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include "ui/gfx/x/x11_types.h"

namespace ui {

bool IsAltPressed() {
  XDisplay* display = gfx::GetXDisplay();
  if (display) {
    char keys[32];  // a bit array of each key currently pressed
    XQueryKeymap(display, keys);
    const KeyCode alt_l = XKeysymToKeycode(display, XK_Alt_L);
    const KeyCode alt_r = XKeysymToKeycode(display, XK_Alt_R);

    return ((keys[alt_l / 8] >> (alt_l % 8)) & 1) ||
           ((keys[alt_r / 8] >> (alt_r % 8)) & 1);
  }

  return false;
}

}  // namespace ui
