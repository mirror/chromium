// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.InputDevice;
import android.view.MotionEvent;

/**
 * Motion modifier that is used to change the mapping between
 * the positions on the screen and touch events.
 */
public interface MotionEventModifier {
   MotionEvent getCalibratedEvent(MotionEvent event);
}
