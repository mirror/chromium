// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.incognito;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for IncognitoTabSnapshotObserver.java.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class IncognitoTabSnapshotObserverTest {
    private IncognitoTabSnapshotObserver mObserver;
    private WindowManager.LayoutParams mParams;

    @Mock
    Window mWindow;

    @Mock
    TabModelSelector mSelector;

    @Mock
    TabModel mTabModel;

    @Before
    public void before() {
        MockitoAnnotations.initMocks(this);

        mParams = new LayoutParams();
    }

    @Test
    public void testUpdateIncognitoStateIncognitoAndEarlyReturn() {
        mParams.flags = WindowManager.LayoutParams.FLAG_SECURE;
        doReturn(mParams).when(mWindow).getAttributes();

        mObserver = new TestIncognitoTabSnapshotObserver(mWindow, mSelector, true);
        mObserver.updateIncognitoState();
        verify(mWindow, never()).setAttributes(any());
        verify(mWindow, never()).clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
        Assert.assertEquals(
                "Flag should be secure", WindowManager.LayoutParams.FLAG_SECURE, mParams.flags);
    }

    @Test
    public void testUpdateIncognitoStateNotIncognitoAndEarlyReturn() {
        mParams.flags = 0;
        doReturn(mParams).when(mWindow).getAttributes();

        mObserver = new TestIncognitoTabSnapshotObserver(mWindow, mSelector, false);
        mObserver.updateIncognitoState();
        verify(mWindow, never()).setAttributes(any());
        Assert.assertEquals("Flag should be zero", 0, mParams.flags);
    }

    @Test
    public void testUpdateIncognitoStateSwitchingToIncognito() {
        mParams.flags = 0;
        doReturn(mParams).when(mWindow).getAttributes();

        mObserver = new TestIncognitoTabSnapshotObserver(mWindow, mSelector, true);
        mObserver.updateIncognitoState();
        verify(mWindow, atLeastOnce()).setAttributes(mParams);
        Assert.assertEquals(
                "Flag should be secure", WindowManager.LayoutParams.FLAG_SECURE, mParams.flags);
    }

    @Test
    public void testUpdateIncognitoStateSwitchingToNonIncognito() {
        mParams.flags = WindowManager.LayoutParams.FLAG_SECURE;
        doReturn(mParams).when(mWindow).getAttributes();

        mObserver = new TestIncognitoTabSnapshotObserver(mWindow, mSelector, false);
        mObserver.updateIncognitoState();
        verify(mWindow, atLeastOnce()).clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
    }

    @Test
    public void testIsShowingIncognitoNotInOverviewMode() {
        mObserver = new IncognitoTabSnapshotObserver(mWindow, mSelector);
        mObserver.setInOverViewMode(false);
        doReturn(mTabModel).when(mSelector).getCurrentModel();
        doReturn(true).when(mTabModel).isIncognito();
        Assert.assertTrue("Should return true", mObserver.isShowingIncognito());

        verify(mSelector, never()).getModel(true);
    }

    @Test
    public void testIsShowingIncognitoInOverviewMode() {
        mObserver = new IncognitoTabSnapshotObserver(mWindow, mSelector);
        mObserver.setInOverViewMode(false);
        doReturn(mTabModel).when(mSelector).getCurrentModel();
        doReturn(true).when(mTabModel).isIncognito();
        Assert.assertTrue("Should return true", mObserver.isShowingIncognito());

        verify(mSelector, never()).getModel(true);
    }

    @Test
    public void testInOverviewModeWithIncognitoTab() {
        mObserver = new IncognitoTabSnapshotObserver(mWindow, mSelector);
        mObserver.setInOverViewMode(true);

        doReturn(mTabModel).when(mSelector).getCurrentModel();
        doReturn(false).when(mTabModel).isIncognito();
        doReturn(mTabModel).when(mSelector).getModel(true);
        doReturn(1).when(mTabModel).getCount();
        Assert.assertTrue("Should return true", mObserver.isShowingIncognito());

        verify(mTabModel, atLeastOnce()).getCount();
    }

    @Test
    public void testInOverviewModeWithNoIncognitoTab() {
        mObserver = new IncognitoTabSnapshotObserver(mWindow, mSelector);
        mObserver.setInOverViewMode(true);

        doReturn(mTabModel).when(mSelector).getCurrentModel();
        doReturn(false).when(mTabModel).isIncognito();
        doReturn(mTabModel).when(mSelector).getModel(true);
        doReturn(0).when(mTabModel).getCount();
        Assert.assertFalse("Should return false", mObserver.isShowingIncognito());

        verify(mTabModel, atLeastOnce()).getCount();
    }

    class TestIncognitoTabSnapshotObserver extends IncognitoTabSnapshotObserver {
        private boolean mIsShowingIncognito;

        public TestIncognitoTabSnapshotObserver(
                Window window, TabModelSelector selector, boolean isShowingIncognito) {
            super(window, selector);
            mIsShowingIncognito = isShowingIncognito;
        }

        @Override
        boolean isShowingIncognito() {
            return mIsShowingIncognito;
        }
    }
}