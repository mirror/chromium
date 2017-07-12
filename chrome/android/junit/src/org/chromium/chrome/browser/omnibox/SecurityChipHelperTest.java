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

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

/**
 * Unit tests for {@link LocationBarLayout} class.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public final class SecurityChipHelperTest {
    private static final List<Consumer<SecurityChipHelper.ToolbarState>> DEVICE_SIZES =
            Arrays.asList(ts -> {
                    /* For testing phone scenarios. */
                ts.isSmallDevice = true;
            }, ts -> {
                    /* For testing tablet scenarios. */
                    ts.isSmallDevice = false;
                });

    private static final List<Consumer<SecurityChipHelper.ToolbarState>> SECURE_LEVELS =
            Arrays.asList(ts -> {
                ts.securityLevel = ConnectionSecurityLevel.SECURE;
            }, ts -> {
                    ts.securityLevel = ConnectionSecurityLevel.EV_SECURE;
                });

    private static final List<Consumer<SecurityChipHelper.ToolbarState>> DANGEROUS_LEVELS =
            Arrays.asList(ts -> {
                ts.securityLevel = ConnectionSecurityLevel.DANGEROUS;
            });

    private static final List<Consumer<SecurityChipHelper.ToolbarState>> WARNING_LEVELS =
            Arrays.asList(ts -> {
                ts.securityLevel = ConnectionSecurityLevel.HTTP_SHOW_WARNING;
            }, ts -> {
                    ts.securityLevel = ConnectionSecurityLevel.SECURITY_WARNING;
                });

    private static final int DARK_THEME_COLOR = 0x720e9e;
    private static final int LIGHT_THEME_COLOR = 0xf9c81e;

    @Mock
    private Resources mResources;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        // Two parameters to getColor presume Robolectric running with M+.
        doReturn(0xf2f2f2).when(mResources).getColor(eq(R.color.default_primary_color), eq(null));
    }

    private SecurityChipHelper.ToolbarState getDefaultToolbarState() {
        SecurityChipHelper.ToolbarState toolbarState = new SecurityChipHelper.ToolbarState();
        toolbarState.hasTab = true;
        toolbarState.isIncognito = false;
        toolbarState.isOfflinePage = false;
        toolbarState.isSmallDevice = true;
        toolbarState.isUrlBarFocused = false;
        toolbarState.isUsingBrandColor = false;
        toolbarState.primaryColor =
                ApiCompatibilityUtils.getColor(mResources, R.color.default_primary_color);
        toolbarState.securityLevel = ConnectionSecurityLevel.NONE;
        return toolbarState;
    }

    private SecurityChipHelper.ChipState getExpectedChipState() {
        SecurityChipHelper.ChipState chipState = new SecurityChipHelper.ChipState();
        chipState.showSecurityIcon = false;
        chipState.securityIconResourceId = 0;
        chipState.securityIconColorStateListId = 0;
        chipState.verboseStatusVisibility = View.GONE;
        chipState.verboseStatusColorId = 0;
        chipState.verboseStatusSeparatorColorId = 0;
        chipState.shouldColorHttpsScheme = true;
        chipState.useDarkForegroundColors = true;
        return chipState;
    }

    private Consumer<SecurityChipHelper.ChipState> showIconWithDarkTint(int iconResourceId) {
        return cs -> {
            cs.showSecurityIcon = true;
            cs.securityIconResourceId = iconResourceId;
            cs.securityIconColorStateListId = R.color.dark_mode_tint;
        };
    }

    private Consumer<SecurityChipHelper.ChipState> showIconWithColorList(
            int iconResourceId, int colorStateListResourceId) {
        return cs -> {
            cs.showSecurityIcon = true;
            cs.securityIconResourceId = iconResourceId;
            cs.securityIconColorStateListId = colorStateListResourceId;
        };
    }

    Map<Consumer<SecurityChipHelper.ToolbarState>, Consumer<SecurityChipHelper.ChipState>>
            getListOfSecurityScenarios() {
        Map<Consumer<SecurityChipHelper.ToolbarState>, Consumer<SecurityChipHelper.ChipState>>
                scenarios = new HashMap<>();

        // First two scenarios get differnet handling depending on whether device is small or not.
        // Line breaks are expected by presubmit.
        scenarios.put(ts -> {
        }, cs -> {
            });
        scenarios.put(ts -> {
            ts.isSmallDevice = false;
        }, showIconWithColorList(R.drawable.omnibox_info, R.color.dark_mode_tint));

        // Everything else expects same output independently of the size, hence using streams to
        // create relevant combinations.

        // Intermediate result of collect is a map, mapping Consumer<ToolbarState> to
        // Consumer<ChipState>, which are transformations done to input and expected state, before
        // verification is done.

        // First set of scenarios is: warning level on both device sizes.
        scenarios.putAll(DEVICE_SIZES.stream()
                .flatMap(x -> WARNING_LEVELS.stream().map(y -> x.andThen(y)))
                .collect(Collectors.toMap(
                        Function.identity(),
                        x -> showIconWithColorList(R.drawable.omnibox_info,
                                R.color.dark_mode_tint))));

        // Next set: dangerous level on both device sizes.
        scenarios.putAll(DEVICE_SIZES.stream()
                .flatMap(x -> DANGEROUS_LEVELS.stream().map(y -> x.andThen(y)))
                .collect(Collectors.toMap(Function.identity(),
                        x -> showIconWithColorList(R.drawable.omnibox_https_invalid,
                                R.color.google_red_700))));

        // Next set: Secure levels on both device sizes.
        scenarios.putAll(DEVICE_SIZES.stream()
                .flatMap(x -> SECURE_LEVELS.stream().map(y -> x.andThen(y)))
                .collect(Collectors.toMap(Function.identity(),
                        x -> showIconWithColorList(R.drawable.omnibox_https_valid,
                                R.color.google_green_700))));

        // Next set: Scenarios for showing offline page.
        // There is more setup, because we are showing a verbose status.
        scenarios.putAll(DEVICE_SIZES.stream()
                .map(x -> x.andThen(ts -> {
                    ts.isOfflinePage = true;
                })).collect(Collectors.toMap(Function.identity(), x -> cs -> {
                    cs.showSecurityIcon = true;
                    cs.securityIconResourceId = R.drawable.offline_pin_round;
                    cs.securityIconColorStateListId = R.color.dark_mode_tint;
                    cs.verboseStatusVisibility = View.VISIBLE;
                    cs.verboseStatusColorId = R.color.locationbar_status_color;
                    cs.verboseStatusSeparatorColorId = R.color.locationbar_status_separator_color;
                })));

        return scenarios;
    }

    @Test
    public void calculateSecurityChipStateForDefaultColors() {
        Map<Consumer<SecurityChipHelper.ToolbarState>, Consumer<SecurityChipHelper.ChipState>>
                scenarios = getListOfSecurityScenarios();

        // Remider: x is Consumer<ToolbarState>, while y is Consumer<ChipState>.
        scenarios.forEach((x, y) -> runCalculateSecurityChipStateTest(x, y));
    }

    @Test
    public void calculateSecurityChipStateForIncognito() {
        Map<Consumer<SecurityChipHelper.ToolbarState>, Consumer<SecurityChipHelper.ChipState>>
                scenarios = getListOfSecurityScenarios();

        // Modifies scenarios for incognito case.
        // Remider: x is Consumer<ToolbarState>, while y is Consumer<ChipState>.
        // We are making updates to properly transform input (x) and expected outcome (y).
        scenarios.forEach(
                (x, y) -> runCalculateSecurityChipStateTest(x.andThen(ts -> {
                    ts.isIncognito = true;
                }), y.andThen(cs -> {
                    cs.shouldColorHttpsScheme = false;
                    cs.useDarkForegroundColors = false;
                    if (cs.showSecurityIcon) {
                        cs.securityIconColorStateListId = R.color.light_mode_tint;
                        if (cs.verboseStatusVisibility == View.VISIBLE) {
                            cs.verboseStatusColorId =
                                    R.color.locationbar_status_color_light;
                            cs.verboseStatusSeparatorColorId =
                                    R.color.locationbar_status_separator_color_light;
                        }
                    }
                })));
    }

    @Test
    public void calculateSecurityChipStateForDarkThemePage() {
        Map<Consumer<SecurityChipHelper.ToolbarState>, Consumer<SecurityChipHelper.ChipState>>
                scenarios = getListOfSecurityScenarios();

        // Modifies scenarios for dark themed page.
        // Remider: x is Consumer<ToolbarState>, while y is Consumer<ChipState>.
        // We are making updates to properly transform input (x) and expected outcome (y).
        scenarios.forEach(
                (x, y) -> runCalculateSecurityChipStateTest(x.andThen(ts -> {
                    ts.primaryColor = DARK_THEME_COLOR;
                    ts.isUsingBrandColor = true;
                }), y.andThen(cs -> {
                    cs.shouldColorHttpsScheme = false;
                    cs.useDarkForegroundColors = false;
                    if (cs.showSecurityIcon) {
                        cs.securityIconColorStateListId = R.color.light_mode_tint;
                        if (cs.verboseStatusVisibility == View.VISIBLE) {
                            cs.verboseStatusColorId =
                                    R.color.locationbar_status_color_light;
                            cs.verboseStatusSeparatorColorId =
                                    R.color.locationbar_status_separator_color_light;
                        }
                    }
                })));
    }

    @Test
    public void calculateSecurityChipStateForLightThemePage() {
        Map<Consumer<SecurityChipHelper.ToolbarState>, Consumer<SecurityChipHelper.ChipState>>
                scenarios = getListOfSecurityScenarios();

        // Modifies scenarios for light themed page.
        // Remider: x is Consumer<ToolbarState>, while y is Consumer<ChipState>.
        // We are making updates to properly transform input (x) and expected outcome (y).
        scenarios.forEach(
                (x, y) -> runCalculateSecurityChipStateTest(x.andThen(ts -> {
                    ts.primaryColor = LIGHT_THEME_COLOR;
                    ts.isUsingBrandColor = true;
                }), y.andThen(cs -> {
                    cs.shouldColorHttpsScheme = false;
                    cs.useDarkForegroundColors = true;
                    if (cs.showSecurityIcon) {
                        cs.securityIconColorStateListId = R.color.dark_mode_tint;
                    }
                })));
    }

    private void runCalculateSecurityChipStateTest(
            Consumer<SecurityChipHelper.ToolbarState> inputTransformation,
            Consumer<SecurityChipHelper.ChipState> expectedResultTransformation) {
        // Transformation of input and expected output.
        SecurityChipHelper.ToolbarState toolbarState = getDefaultToolbarState();
        inputTransformation.accept(toolbarState);

        SecurityChipHelper.ChipState expectedChipState = getExpectedChipState();
        expectedResultTransformation.accept(expectedChipState);

        runCalculateSecurityChipStateTest(toolbarState, expectedChipState);
    }

    private void runCalculateSecurityChipStateTest(SecurityChipHelper.ToolbarState toolbarState,
            SecurityChipHelper.ChipState expectedChipState) {
        // Running function under test.
        SecurityChipHelper.ChipState actualChipState =
                new SecurityChipHelper(mResources).calculateSecurityChipState(toolbarState);

        String caseString = dumpCase(toolbarState, expectedChipState);
        assertEquals(caseString + "show security icon ", expectedChipState.showSecurityIcon,
                actualChipState.showSecurityIcon);
        assertEquals(caseString + "security icon resource id ",
                expectedChipState.securityIconResourceId, actualChipState.securityIconResourceId);
        assertEquals(caseString + "security icon color list id ",
                expectedChipState.securityIconColorStateListId,
                actualChipState.securityIconColorStateListId);

        assertEquals(caseString + "verbose status visibility ",
                expectedChipState.verboseStatusVisibility, actualChipState.verboseStatusVisibility);
        assertEquals(caseString + "verbose status color id ",
                expectedChipState.verboseStatusColorId, actualChipState.verboseStatusColorId);
        assertEquals(caseString + "verbose status separator color id ",
                expectedChipState.verboseStatusSeparatorColorId,
                actualChipState.verboseStatusSeparatorColorId);

        assertEquals(caseString + "should emphasize URL ", expectedChipState.shouldColorHttpsScheme,
                actualChipState.shouldColorHttpsScheme);
        assertEquals(caseString + "use dark foreground color ",
                expectedChipState.useDarkForegroundColors, actualChipState.useDarkForegroundColors);
    }

    private String dumpCase(
            SecurityChipHelper.ToolbarState toolbarState, SecurityChipHelper.ChipState chipState) {
        return String.format(
                "Toolbar state:\n\thasTab = %b,\n\tisIncognito = %b,\n\tisOfflinePage = %b,\n\t"
                        + "isSmallDevice = %b,\n\tisUrlBarFocused = %b,\n\t"
                        + "isUsingBrandColor = %b,\n\tprimaryColor = %d,\n\tsecurityLevel = %d\n"
                        + "Expected chip state:\n\tshowSecurityIcon = %b,\n\t"
                        + "securityIconResourceId = %d,\n\tsecurityIconColorStateListId = %d,\n\t"
                        + "verboseStatusVisibility = %d,\n\tverboseStatusColorId = %d,\n\t"
                        + "verboseStatusSeparatorColorId = %d,\n\tshouldColorHttpsScheme = %b,\n\t"
                        + "useDarkForegroundColors = %b\n",
                toolbarState.hasTab, toolbarState.isIncognito, toolbarState.isOfflinePage,
                toolbarState.isSmallDevice, toolbarState.isUrlBarFocused,
                toolbarState.isUsingBrandColor, toolbarState.primaryColor,
                toolbarState.securityLevel, chipState.showSecurityIcon,
                chipState.securityIconResourceId, chipState.securityIconColorStateListId,
                chipState.verboseStatusVisibility, chipState.verboseStatusColorId,
                chipState.verboseStatusSeparatorColorId, chipState.shouldColorHttpsScheme,
                chipState.useDarkForegroundColors);
    }
}