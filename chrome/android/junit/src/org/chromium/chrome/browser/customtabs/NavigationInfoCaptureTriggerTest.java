// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.util.Feature;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Tests for {@link NavigationInfoCaptureTrigger}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class NavigationInfoCaptureTriggerTest {
    @Mock
    private NavigationInfoCaptureTrigger.Delegate mDelegate;
    private NavigationInfoCaptureTrigger mTrigger;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        when(mDelegate.captureNavigationInfo(any())).thenReturn(Boolean.TRUE);
        mTrigger = new NavigationInfoCaptureTrigger(mDelegate);
    }

    /**
     * Tests the normal flow where onload is called, then first meaningful paint happens soon
     * after. We want the screen to be captured twice, once for the initial capture, once for the
     * first meaningful paint capture.
     */
    @Test
    @Feature({"CustomTabs"})
    public void testNormalFlow() {
        mTrigger.onLoadFinished(null);
        mTrigger.onFirstMeaningfulPaint(null);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();

        verify(mDelegate, times(2)).captureNavigationInfo(any());
    }

    /**
     * Tests the flow where onload is called and first meaningful paint either isn't called or is
     * called a long time after. We expect capture to be called twice, once for the initial and
     * once for the backup.
     */
    @Test
    @Feature({"CustomTabs"})
    public void testDelayedFmp() {
        mTrigger.onLoadFinished(null);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();

        verify(mDelegate, times(2)).captureNavigationInfo(any());

        mTrigger.onFirstMeaningfulPaint(null);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        verify(mDelegate, times(2)).captureNavigationInfo(any());
    }

    /**
     * Tests the flow where first meaningful paint is called before onload. The screen should only
     * be captured once, after the first meaningful paint.
     */
    @Test
    @Feature({"CustomTabs"})
    public void testDelayedOnload() {
        mTrigger.onFirstMeaningfulPaint(null);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();

        verify(mDelegate, times(1)).captureNavigationInfo(any());

        mTrigger.onLoadFinished(null);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();

        verify(mDelegate, times(1)).captureNavigationInfo(any());
    }

    /**
     * Tests that pending capture tasks are cancelled when the page navigates.
     */
    @Test
    @Feature({"CustomTabs"})
    public void testCancelOnNavigation() {
        mTrigger.onLoadFinished(null);
        mTrigger.onFirstMeaningfulPaint(null);

        mTrigger.onNewNavigation();
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        verify(mDelegate, times(0)).captureNavigationInfo(any());
    }

    /**
     * Tests that navigation resets the state.
     */
    @Test
    @Feature({"CustomTabs"})
    public void testResetOnNavigation() {
        testNormalFlow();

        mTrigger.onNewNavigation();

        clearInvocations(mDelegate); // Clears the mock so the verifies in the original test work.
        testDelayedFmp();

        mTrigger.onNewNavigation();

        clearInvocations(mDelegate);
        testDelayedOnload();
    }

    /**
     * Tests that we capture only on the first FMP.
     */
    @Test
    @Feature({"CustomTabs"})
    public void testMultipleFmps() {
        mTrigger.onFirstMeaningfulPaint(null);
        mTrigger.onFirstMeaningfulPaint(null);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();

        verify(mDelegate, times(1)).captureNavigationInfo(any());
    }
}
