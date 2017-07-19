// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.ScaleDrawable;
import android.view.Gravity;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.ui.base.LocalizationUtils;

/**
 * When suggestions cards are displayed on a white background, thumbnails with white backgrounds
 * have a gradient overlaid to provide contrast at the edge of the cards.
 */
public class ThumbnailGradient {
    /** If all the RGB values of a pixel are greater than this value, it is counted as 'light'. */
    private static final int LIGHT_PIXEL_THRESHOLD = 0xcc;

    /** The percent of the border pictures that need to be 'light' for a Bitmap to be 'light'. */
    private static final float PIXEL_BORDER_RATIO = 0.4f;

    /** The alpha value of the ThumbnailGradient (from 0 to 255). */
    private static final int GRADIENT_ALPHA = 16;

    /**
     * Whether a ThumbnailGradient should be applied. This checks whether the feature is enabled (it
     * is only enabled with some suggestions layouts) and whether the Bitmap is light enough to
     * require the gradient.
     */
    public static boolean shouldApply(Bitmap bitmap) {
        return isThumbnailGradientEnabled() && hasLightBorder(bitmap);
    }

    /**
     * Adds a gradient to the Drawable. You should first check if this is required by calling
     * {@link #shouldApply(Bitmap)}.
     */
    public static Drawable apply(Drawable drawable, Resources resources) {
        Drawable gradient =
                ApiCompatibilityUtils.getDrawable(resources, R.drawable.snippet_thumbnail_gradient);
        gradient.setAlpha(GRADIENT_ALPHA);

        if (shouldMirrorGradient()) {
            gradient = new ScaleDrawable(gradient, Gravity.CENTER, -1f, 1f);
        }

        Drawable[] layers = {drawable, gradient};
        return new LayerDrawable(layers);
    }

    private static boolean isThumbnailGradientEnabled() {
        return ChromeFeatureList.isEnabled(ChromeFeatureList.CONTENT_SUGGESTIONS_LARGE_THUMBNAIL)
                || ChromeFeatureList.isEnabled(ChromeFeatureList.SUGGESTIONS_HOME_MODERN_LAYOUT);
    }

    /**
     * Whether a Bitmap has a light border.
     */
    private static boolean hasLightBorder(Bitmap bitmap) {
        int lightPixels = 0;

        for (int x = 0; x < bitmap.getWidth(); x++) {
            int top = bitmap.getPixel(x, 0);
            int bottom = bitmap.getPixel(x, bitmap.getHeight() - 1);

            if (isPixelLight(top)) lightPixels++;
            if (isPixelLight(bottom)) lightPixels++;
        }

        // Avoid counting the corner pixels twice.
        for (int y = 1; y < bitmap.getHeight() - 1; y++) {
            int left = bitmap.getPixel(0, y);
            int right = bitmap.getPixel(bitmap.getWidth() - 1, 0);

            if (isPixelLight(left)) lightPixels++;
            if (isPixelLight(right)) lightPixels++;
        }

        return lightPixels > getBorderSize(bitmap) * PIXEL_BORDER_RATIO;
    }

    /**
     * Whether a pixel counts as light.
     */
    private static boolean isPixelLight(int color) {
        return Color.red(color) > LIGHT_PIXEL_THRESHOLD && Color.blue(color) > LIGHT_PIXEL_THRESHOLD
                && Color.green(color) > LIGHT_PIXEL_THRESHOLD;
    }

    /**
     * Gets the number of pixels in the border of a bitmap.
     */
    private static int getBorderSize(Bitmap bitmap) {
        // The length of each side minus the 4 corner pixels which are counted twice.
        return (bitmap.getWidth() + bitmap.getHeight()) * 2 - 4;
    }

    /**
     * The gradient should come from the top outside corner of the card.
     */
    private static boolean shouldMirrorGradient() {
        assert isThumbnailGradientEnabled();

        // The drawable is set up correctly for the modern layout, but needs to be flipped for the
        // large thumbnail layout.
        boolean modern =
                ChromeFeatureList.isEnabled(ChromeFeatureList.SUGGESTIONS_HOME_MODERN_LAYOUT);

        // TODO(peconn): Check that things aren't automatically flipped in RTL.
        // Things then need to be flipped if we are in RTL.
        boolean rtl = LocalizationUtils.isLayoutRtl();

        return modern == rtl;
    }
}
