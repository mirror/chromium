// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.content.res.Resources;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.components.security_state.ConnectionSecurityLevel;

/**
 * Class capturing the calculation of security chip state.
 */
public class SecurityChipHelper {
    /**
     * Class reprenenting a state of the toolbar, for which an expected state of security chip will
     * be calculated.
     */
    static class ToolbarState {
        public boolean hasTab;
        public boolean isIncognito;
        public boolean isOfflinePage;
        public boolean isSmallDevice;
        public boolean isUrlBarFocused;
        public boolean isUsingBrandColor;
        public int primaryColor;
        public int securityLevel;

        @Override
        public boolean equals(Object object) {
            if (!(object instanceof ToolbarState)) return false;

            final ToolbarState other = (ToolbarState) object;
            return hasTab == other.hasTab && isIncognito == other.isIncognito
                    && isOfflinePage == other.isOfflinePage && isSmallDevice == other.isSmallDevice
                    && isUrlBarFocused == other.isUrlBarFocused
                    && isUsingBrandColor == other.isUsingBrandColor
                    && primaryColor == other.primaryColor && securityLevel == other.securityLevel;
        }

        @Override
        public int hashCode() {
            int hash = 31;
            hash = 37 * hash + (hasTab ? 1 : 0);
            hash = 37 * hash + (isIncognito ? 1 : 0);
            hash = 37 * hash + (isOfflinePage ? 1 : 0);
            hash = 37 * hash + (isSmallDevice ? 1 : 0);
            hash = 37 * hash + (isUrlBarFocused ? 1 : 0);
            hash = 37 * hash + (isUsingBrandColor ? 1 : 0);
            hash = 37 * hash + primaryColor;
            hash = 37 * hash + securityLevel;
            return hash;
        }
    }

    /**
     * Class representing a state of the security chip.
     * TODO(fgorski): consider extending it to cover navigation icon (and animations).
     */
    static class ChipState {
        public boolean showSecurityIcon;
        public int securityIconResourceId;
        public int securityIconColorStateListId;

        public int verboseStatusVisibility;
        public int verboseStatusColorId;
        public int verboseStatusSeparatorColorId;

        public boolean shouldColorHttpsScheme;
        public boolean useDarkForegroundColors;
    }

    private Resources mResources;

    public SecurityChipHelper(Resources resources) {
        mResources = resources;
    }

    /**
     * @param securityLevel The security level for which the color will be returned.
     * @param isIncognito We are currently off the record.
     * @param isDefaultToolbarColor The color used is a default toolbar color.
     * @param isUsingDarkTheme Whether the page is using a theme classified as dark, which makes it
     *     require light colors for the icons.
     * @param isOmniboxOpaque Whether the omnibox is an opaque color.
     * @return A resource ID of for the color state list, or 0 if nothing was selected.
     */
    private static int getColorStateListResource(int securityLevel, boolean isIncognito,
            boolean isDefaultToolbarColor, boolean isUsingDarkTheme, boolean isOmniboxOpaque) {
        // TODO(fgorski): this likely maps to !useDarkColors from LocationBarLayout.
        if (isIncognito || isUsingDarkTheme) return R.color.light_mode_tint;

        // For theme colors which are not dark and are also not light enough to warrant an opaque
        // URL bar, use dark icons.
        if (!isDefaultToolbarColor && !isOmniboxOpaque) return R.color.dark_mode_tint;

        if (securityLevel == ConnectionSecurityLevel.DANGEROUS) return R.color.google_red_700;

        if (securityLevel == ConnectionSecurityLevel.SECURE
                || securityLevel == ConnectionSecurityLevel.EV_SECURE) {
            return R.color.google_green_700;
        }

        return R.color.dark_mode_tint;
    }

    /**
     * @param isIncognito Whether toolbar is shown in incognito mode.
     * @param isUsingBrandColor Whether the page shown uses brand color.
     * @return Whether HTTPS scheme should be colored (red or green) or not.
     */
    private static boolean shouldColorHttpsScheme(boolean isIncognito, boolean isUsingBrandColor) {
        return !isIncognito && !isUsingBrandColor;
    }

    /**
     * Update visibility of the verbose status based on the button type and focus state of the
     * omnibox.
     */
    private static int getVerboseStatusVisibility(boolean isUrlBarFocused, boolean isOfflinePage) {
        // Because is offline page is cleared a bit slower, we also ensure that connection security
        // level is NONE or HTTP_SHOW_WARNING (http://crbug.com/671453).
        boolean verboseStatusVisible = !isUrlBarFocused && isOfflinePage;
        return verboseStatusVisible ? View.VISIBLE : View.GONE;
    }

    /**
     * Gets the color of location bar status.
     * @param useDarkForegroundcolors Whether foreground colors should be dark (indicating light
     *     background).
     * @return Resource ID of status  color.
     */
    private static int getVerboseStatusColorId(boolean useDarkForegroundColors) {
        return useDarkForegroundColors ? R.color.locationbar_status_color
                                       : R.color.locationbar_status_color_light;
    }

    /**
     * Gets the color of location bar status separator.
     * @param useDarkForegroundcolors Whether foreground colors should be dark (indicating light
     *     background).
     * @return Resource ID of status separator color.
     */
    private static int getVerboseStatusSeparatorColorId(boolean useDarkForegroundColors) {
        return useDarkForegroundColors ? R.color.locationbar_status_separator_color
                                       : R.color.locationbar_status_separator_color_light;
    }

    /**
     * Determines whether dark colors should be used in the foreground.
     * @param toolbarState Toolbar state for which the calculation is made.
     * @return Whether {@link LocationBar#mUseDarkColors} has been updated.
     */
    private static boolean getUseDarkColors(ToolbarState toolbarState) {
        if (toolbarState.isIncognito) return false;

        if (!toolbarState.hasTab || !toolbarState.isUsingBrandColor
                || toolbarState.isUrlBarFocused) {
            return true;
        }

        return !ColorUtils.shouldUseLightForegroundOnBackground(toolbarState.primaryColor);
    }

    /**
     * Determines the icon that should be displayed for the current security level.
     * @param securityLevel The security level for which the resource will be returned.
     * @param isSmallDevice Whether the device form factor is small (like a phone) or large
     * (like a tablet).
     * @param isOfflinePage Whether the page for which the icon is shown is an offline page.
     * @return The resource ID of the icon that should be displayed, 0 if no icon should show.
     */
    private static int getSecurityIconResource(
            int securityLevel, boolean isSmallDevice, boolean isOfflinePage) {
        if (isOfflinePage) {
            return R.drawable.offline_pin_round;
        }
        switch (securityLevel) {
            case ConnectionSecurityLevel.NONE:
                return isSmallDevice ? 0 : R.drawable.omnibox_info;
            case ConnectionSecurityLevel.HTTP_SHOW_WARNING:
                return R.drawable.omnibox_info;
            case ConnectionSecurityLevel.SECURITY_WARNING:
                return R.drawable.omnibox_info;
            case ConnectionSecurityLevel.DANGEROUS:
                return R.drawable.omnibox_https_invalid;
            case ConnectionSecurityLevel.SECURE:
            case ConnectionSecurityLevel.EV_SECURE:
                return R.drawable.omnibox_https_valid;
            default:
                assert false;
        }
        return 0;
    }

    /**
     * Calculates state of security chip based on the current state collected from the toolbar.
     * @param toolbarState Current state as collected from the toolbar.
     * @return Security chip state corresponding to provided toolbar state.
     */
    public ChipState calculateSecurityChipState(ToolbarState toolbarState) {
        ChipState chipState = new ChipState();

        chipState.securityIconResourceId = getSecurityIconResource(
                toolbarState.securityLevel, toolbarState.isSmallDevice, toolbarState.isOfflinePage);

        chipState.showSecurityIcon = chipState.securityIconResourceId != 0;
        if (chipState.showSecurityIcon) {
            boolean isDefaultToolbarColor =
                    ColorUtils.isUsingDefaultToolbarColor(mResources, toolbarState.primaryColor);
            boolean isUsingDarkTheme =
                    ColorUtils.shouldUseLightForegroundOnBackground(toolbarState.primaryColor);
            boolean isOmniboxOpaque =
                    ColorUtils.shouldUseOpaqueTextboxBackground(toolbarState.primaryColor);

            chipState.securityIconColorStateListId =
                    getColorStateListResource(toolbarState.securityLevel, toolbarState.isIncognito,
                            isDefaultToolbarColor, isUsingDarkTheme, isOmniboxOpaque);
        }

        boolean useDarkForegroundColors = getUseDarkColors(toolbarState);
        chipState.useDarkForegroundColors = useDarkForegroundColors;

        chipState.verboseStatusVisibility = getVerboseStatusVisibility(
                toolbarState.isUrlBarFocused, toolbarState.isOfflinePage);

        if (chipState.verboseStatusVisibility == View.VISIBLE) {
            chipState.verboseStatusColorId = getVerboseStatusColorId(useDarkForegroundColors);
            chipState.verboseStatusSeparatorColorId =
                    getVerboseStatusSeparatorColorId(useDarkForegroundColors);
        }

        chipState.shouldColorHttpsScheme =
                shouldColorHttpsScheme(toolbarState.isIncognito, toolbarState.isUsingBrandColor);

        return chipState;
    }
}