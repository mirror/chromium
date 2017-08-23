// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.animation;

import static org.junit.Assert.assertEquals;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.content.Context;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.compositor.layouts.Layout;
import org.chromium.chrome.browser.compositor.layouts.LayoutUpdateHost;
import org.chromium.chrome.browser.compositor.layouts.components.LayoutTab;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for the {@link CompositorAnimator} class.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public final class CompositorAnimatorTest {
    /** A mock implementation of {@link LayoutUpdateHost} that tracks update requests. */
    private static class MockLayoutUpdateHost implements LayoutUpdateHost {
        private final CallbackHelper mUpdateCallbackHelper = new CallbackHelper();

        @Override
        public void requestUpdate() {
            mUpdateCallbackHelper.notifyCalled();
        }

        @Override
        public void startHiding(int nextTabId, boolean hintAtTabSelection) {}

        @Override
        public void doneHiding() {}

        @Override
        public void doneShowing() {}

        @Override
        public boolean isActiveLayout(Layout layout) {
            return true;
        }

        @Override
        public void initLayoutTabFromHost(final int tabId) {}

        @Override
        public LayoutTab createLayoutTab(int id, boolean incognito, boolean showCloseButton,
                boolean isTitleNeeded, float maxContentWidth, float maxContentHeight) {
            return null;
        }

        @Override
        public void releaseTabLayout(int id) {}
    }

    /** An animation update listener that counts calls to its methods. */
    private static class TestUpdateListener implements CompositorAnimator.AnimatorUpdateListener {
        private final CallbackHelper mUpdateCallbackHelper = new CallbackHelper();
        private float mLastAnimatedFraction;

        @Override
        public void onAnimationUpdate(CompositorAnimator animator) {
            mLastAnimatedFraction = animator.getAnimatedFraction();
            mUpdateCallbackHelper.notifyCalled();
        }
    }

    /** An animation listener for tracking lifecycle events on an animator. */
    private static class TestAnimatorListener extends AnimatorListenerAdapter {
        private final CallbackHelper mCancelCallbackHelper = new CallbackHelper();
        private final CallbackHelper mEndCallbackHelper = new CallbackHelper();
        private final CallbackHelper mStartCallbackHelper = new CallbackHelper();

        @Override
        public void onAnimationCancel(Animator animation) {
            mCancelCallbackHelper.notifyCalled();
        }

        @Override
        public void onAnimationEnd(Animator animation) {
            mEndCallbackHelper.notifyCalled();
        }

        @Override
        public void onAnimationStart(Animator animation) {
            mStartCallbackHelper.notifyCalled();
        }
    }

    /** A mock {@link LayoutUpdateHost} to track update requests. */
    private MockLayoutUpdateHost mHost;

    /** An Android {@link Context} to use for animator creation. */
    private Context mContext;

    /** The handler that is responsible for managing all {@link CompositorAnimator}s. */
    private CompositorAnimationHandler mHandler;

    /** A listener for updates to an animation. */
    private TestUpdateListener mUpdateListener;

    /** A listener for animation lifecycle events. */
    private TestAnimatorListener mListener;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mHost = new MockLayoutUpdateHost();
        mContext = RuntimeEnvironment.application;

        mHandler = CompositorAnimationHandler.getInstance(mContext);
        mHandler.setUpdateHost(mHost);

        mUpdateListener = new TestUpdateListener();
        mListener = new TestAnimatorListener();
    }

    @Test
    public void testAnimationStarted() {
        CompositorAnimator animator = new CompositorAnimator(mContext);
        animator.setDuration(10);
        animator.addListener(mListener);

        assertEquals("No updates should have been requested.", 0,
                mHost.mUpdateCallbackHelper.getCallCount());
        assertEquals("There should be no active animations.", 0, mHandler.getActiveAnimations());
        assertEquals("The 'start' event should not have been called.", 0,
                mListener.mStartCallbackHelper.getCallCount());

        animator.start();

        assertEquals("One update should have been requested.", 1,
                mHost.mUpdateCallbackHelper.getCallCount());
        assertEquals("There should be one active animation.", 1, mHandler.getActiveAnimations());
        assertEquals("The 'start' event should have been called.", 1,
                mListener.mStartCallbackHelper.getCallCount());
    }

    @Test
    public void testAnimationEnd() {
        CompositorAnimator animator = new CompositorAnimator(mContext);
        animator.setDuration(10);
        animator.addListener(mListener);

        assertEquals("No updates should have been requested.", 0,
                mHost.mUpdateCallbackHelper.getCallCount());
        assertEquals("There should be no active animations.", 0, mHandler.getActiveAnimations());
        assertEquals("The 'end' event should not have been called.", 0,
                mListener.mEndCallbackHelper.getCallCount());

        animator.start();

        assertEquals("One update should have been requested.", 1,
                mHost.mUpdateCallbackHelper.getCallCount());
        assertEquals("There should be one active animation.", 1, mHandler.getActiveAnimations());

        mHandler.pushUpdate(15);

        assertEquals("Two updates should have been requested", 2,
                mHost.mUpdateCallbackHelper.getCallCount());
        assertEquals("There should be no active animations.", 0, mHandler.getActiveAnimations());
        assertEquals("The 'cancel' event should not have been called.", 0,
                mListener.mCancelCallbackHelper.getCallCount());
        assertEquals("The 'end' event should have been called.", 1,
                mListener.mEndCallbackHelper.getCallCount());
    }

    @Test
    public void testAnimationCancel() {
        CompositorAnimator animator = new CompositorAnimator(mContext);
        animator.setDuration(10);
        animator.addListener(mListener);

        assertEquals("No updates should have been requested.", 0,
                mHost.mUpdateCallbackHelper.getCallCount());
        assertEquals("There should be no active animations.", 0, mHandler.getActiveAnimations());
        assertEquals("The 'end' event should not have been called.", 0,
                mListener.mEndCallbackHelper.getCallCount());

        animator.start();

        assertEquals("One update should have been requested.", 1,
                mHost.mUpdateCallbackHelper.getCallCount());
        assertEquals("There should be one active animation.", 1, mHandler.getActiveAnimations());

        animator.cancel();

        assertEquals("There should be no active animations.", 0, mHandler.getActiveAnimations());
        assertEquals("The 'cancel' event should have been called.", 1,
                mListener.mCancelCallbackHelper.getCallCount());
        assertEquals("The 'end' event should have been called.", 1,
                mListener.mEndCallbackHelper.getCallCount());
    }
}
