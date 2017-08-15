// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.view.View;

import org.chromium.base.ContextUtils;

/**
 * A helper class to draw an overlay layer on the top of a view to enable highlighting. The overlay
 * layer can be specified to be a circle or a rectangle.
 */
public class ViewHighlighter {
    /**
     * Create a highlight layer over the view.
     * @param view The view to be highlighted.
     * @param circular Whether the highlight should be a circle or rectangle.
     */
    public static void turnOnHighlight(View view, boolean circular) {
        if (view == null) return;

        Drawable background = (Drawable) view.getBackground();
        if (background == null) return;

        PulseDrawable pulseDrawable = circular
                ? PulseDrawable.createCircle(ContextUtils.getApplicationContext())
                : PulseDrawable.createHighlight();
        LayerDrawable drawable = new LayerDrawable(new Drawable[] {background, pulseDrawable});
        view.setBackground(drawable);
        pulseDrawable.start();
    }

    /**
     * Turns off the highlight from the view. The original background of the view is restored.
     * @param view The associated view.
     */
    public static void turnOffHighlight(View view) {
        if (view == null) return;

        Drawable existingBackground = view.getBackground();
        if (existingBackground instanceof LayerDrawable) {
            LayerDrawable layerDrawable = (LayerDrawable) existingBackground;
            if (layerDrawable.getNumberOfLayers() >= 2) {
                view.setBackground(((LayerDrawable) existingBackground).getDrawable(0));
            } else {
                view.setBackground(null);
            }
        }
    }
}