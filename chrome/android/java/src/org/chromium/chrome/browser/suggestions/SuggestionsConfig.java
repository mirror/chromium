// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.res.Resources;
import android.support.annotation.LayoutRes;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * Provides configuration details for suggestions.
 */
public final class SuggestionsConfig {
    /**
     * Experiment parameter for whether to use the condensed tile layout on small screens.
     */
    private static final String PARAM_CONDENSED_TILE_LAYOUT_FOR_SMALL_SCREENS_ENABLED =
            "condensed_tile_layout_for_small_screens_enabled";

    /**
     * Experiment parameter for whether to use the condensed tile layout on large screens.
     */
    private static final String PARAM_CONDENSED_TILE_LAYOUT_FOR_LARGE_SCREENS_ENABLED =
            "condensed_tile_layout_for_large_screens_enabled";

    private SuggestionsConfig() {}

    /**
     * @return Whether to use the modern layout for suggestions in Chrome Home.
     */
    public static boolean useModern() {
        return ChromeFeatureList.isEnabled(ChromeFeatureList.CHROME_HOME_MODERN_LAYOUT);
    }

    public static boolean useExplore() {
        return ChromeFeatureList.isEnabled(ChromeFeatureList.SITES_EXPLORATION);
    }

    @LayoutRes
    public static int getTileGroupLayout() {
        return useExplore() ? R.layout.site_explore_view : R.layout.suggestions_site_tile_grid;
    }

    /**
     * Returns the size of icons to fetch for tiles. It can be smaller than the size it will end
     * up being displayed, as we allow lower resolution icons in order to show less scrabble tiles.
     */
    public static int getTileIconFetchSize() {
        Resources resources = ContextUtils.getApplicationContext().getResources();

        // On ldpi devices, desiredIconSize could be even smaller than the global limit.
        return Math.min(resources.getDimensionPixelSize(R.dimen.tile_view_icon_size),
                ChromeFeatureList.isEnabled(ChromeFeatureList.NTP_TILES_LOWER_RESOLUTION_FAVICONS)
                        ? TileRenderer.ICON_DECREASED_MIN_SIZE_PX
                        : TileRenderer.ICON_MIN_SIZE_PX);
    }

    /**
     * @param resources The resources to fetch the color from.
     * @return The background color for the suggestions sheet content.
     */
    public static int getBackgroundColor(Resources resources) {
        return useModern()
                ? ApiCompatibilityUtils.getColor(resources, R.color.suggestions_modern_bg)
                : ApiCompatibilityUtils.getColor(resources, R.color.ntp_bg);
    }

    /**
     * Returns the current tile style, that depends on the enabled features and the screen size.
     */
    @TileView.Style
    public static int getTileStyle(UiConfig uiConfig) {
        if (SuggestionsConfig.useModern()) return TileView.Style.MODERN;
        if (FeatureUtilities.isChromeHomeEnabled()) return TileView.Style.CLASSIC;

        if (useCondensedTileLayout(uiConfig.getCurrentDisplayStyle().isSmall())) {
            return TileView.Style.CONDENSED;
        }

        return TileView.Style.CLASSIC;
    }

    private static boolean useCondensedTileLayout(boolean isScreenSmall) {
        if (isScreenSmall) {
            return ChromeFeatureList.getFieldTrialParamByFeatureAsBoolean(
                    ChromeFeatureList.NTP_CONDENSED_TILE_LAYOUT,
                    PARAM_CONDENSED_TILE_LAYOUT_FOR_SMALL_SCREENS_ENABLED, false);
        }

        return ChromeFeatureList.getFieldTrialParamByFeatureAsBoolean(
                ChromeFeatureList.NTP_CONDENSED_TILE_LAYOUT,
                PARAM_CONDENSED_TILE_LAYOUT_FOR_LARGE_SCREENS_ENABLED, false);
    }
}
