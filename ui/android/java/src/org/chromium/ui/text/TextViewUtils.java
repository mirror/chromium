// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.text;

import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.support.annotation.ColorInt;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;

/**
 * Helper methods for text views.
 */
public class TextViewUtils {
    private TextViewUtils() {}

    /**
     * Tints the compound drawables for a {@link TextView} with the given {@code color}.
     * @param textView The {@link TextView} for which to tint the drawables.
     * @param color The tint color.
     */
    public static void applyCompoundDrawableTint(TextView textView, @ColorInt int color) {
        Drawable[] drawables = ApiCompatibilityUtils.getCompoundDrawablesRelative(textView);
        for (Drawable d : drawables) {
            if (d != null) d.setColorFilter(color, PorterDuff.Mode.SRC_IN);
        }
        ApiCompatibilityUtils.setCompoundDrawablesRelative(
                textView, drawables[0], drawables[1], drawables[2], drawables[3]);
    }
}
