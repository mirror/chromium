// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;

import android.content.res.Resources;
import android.view.View;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.chrome.R;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for {@link LocationBarLayout} class.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SecurityChipHelperTest {
    private static final int DARK_THEME_COLOR = 0x720e9e;
    private static final int LIGHT_THEME_COLOR = 0xf9c81e;
    private static final int DEFAULT_PRIMARY_COLOR = 0xf2f2f2;

    @Mock
    private Resources mResources;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        // Two parameters to getColor presume Robolectric running with M+.
        doReturn(DEFAULT_PRIMARY_COLOR)
                .when(mResources)
                .getColor(eq(R.color.default_primary_color), eq(null));
    }

    private SecurityChipHelper.ToolbarState buildBasicToolbarState() {
        SecurityChipHelper.ToolbarState toolbarState = new SecurityChipHelper.ToolbarState();
        toolbarState.hasTab = true;
        toolbarState.isIncognito = false;
        toolbarState.isOfflinePage = false;
        toolbarState.isSmallDevice = true;
        toolbarState.isUrlBarFocused = false;
        toolbarState.isUsingBrandColor = false;
        toolbarState.primaryColor = DEFAULT_PRIMARY_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.NONE;
        return toolbarState;
    }

    @Test
    public void calculateSecurityChipState_tablet_lightTheme_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isOfflinePage = true;
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_lightTheme_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_lightTheme_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isOfflinePage = true;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_lightTheme_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_lightTheme_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_lightTheme() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(false, chipState.showSecurityIcon);
        assertEquals(0, chipState.securityIconResourceId);
        assertEquals(0, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_lightTheme() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_lightTheme_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_lightTheme_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_lightTheme_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_lightTheme_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_lightTheme_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_lightTheme_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_lightTheme_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = LIGHT_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_darkTheme_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isOfflinePage = true;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color_light, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color_light,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_darkTheme_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_darkTheme_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_darkTheme_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_darkTheme_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_darkTheme_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_darkTheme_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_darkTheme_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_darkTheme_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isOfflinePage = true;
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color_light, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color_light,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_darkTheme_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_darkTheme_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_darkTheme() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(false, chipState.showSecurityIcon);
        assertEquals(0, chipState.securityIconResourceId);
        assertEquals(0, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_darkTheme() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_darkTheme_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isUsingBrandColor = true;
        toolbarState.primaryColor = DARK_THEME_COLOR;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.google_green_700, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.google_green_700, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.google_red_700, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isOfflinePage = true;
        toolbarState.isSmallDevice = false;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.google_red_700, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isOfflinePage = true;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(false, chipState.showSecurityIcon);
        assertEquals(0, chipState.securityIconResourceId);
        assertEquals(0, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.google_green_700, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.google_green_700, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.dark_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(true, chipState.shouldColorHttpsScheme);
        assertEquals(true, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_incognito_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_incognito_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isOfflinePage = true;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color_light, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color_light,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_incognito_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_incognito_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_incognito_dangerous() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.DANGEROUS;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_invalid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_incognito_http_show_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_incognito_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_incognito_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_incognito() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(false, chipState.showSecurityIcon);
        assertEquals(0, chipState.securityIconResourceId);
        assertEquals(0, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_incognito_offline() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isOfflinePage = true;
        toolbarState.isSmallDevice = false;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.offline_pin_round, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.VISIBLE, chipState.verboseStatusVisibility);
        assertEquals(R.color.locationbar_status_color_light, chipState.verboseStatusColorId);
        assertEquals(R.color.locationbar_status_separator_color_light,
                chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_incognito() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isSmallDevice = false;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_incognito_security_warning() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_info, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_tablet_incognito_ev_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.isSmallDevice = false;
        toolbarState.securityLevel = ConnectionSecurityLevel.EV_SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }

    @Test
    public void calculateSecurityChipState_phone_incognito_secure() {
        SecurityChipHelper.ToolbarState toolbarState = buildBasicToolbarState();
        toolbarState.isIncognito = true;
        toolbarState.securityLevel = ConnectionSecurityLevel.SECURE;

        SecurityChipHelper.ChipState chipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);
        assertEquals(true, chipState.showSecurityIcon);
        assertEquals(R.drawable.omnibox_https_valid, chipState.securityIconResourceId);
        assertEquals(R.color.light_mode_tint, chipState.securityIconColorStateListId);
        assertEquals(View.GONE, chipState.verboseStatusVisibility);
        assertEquals(0, chipState.verboseStatusColorId);
        assertEquals(0, chipState.verboseStatusSeparatorColorId);
        assertEquals(false, chipState.shouldColorHttpsScheme);
        assertEquals(false, chipState.useDarkForegroundColors);
    }
}
