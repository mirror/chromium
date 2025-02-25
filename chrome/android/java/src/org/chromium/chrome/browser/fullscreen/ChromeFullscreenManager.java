// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.fullscreen;

import android.app.Activity;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ApplicationStatus.ActivityStateListener;
import org.chromium.base.ApplicationStatus.WindowFocusChangedListener;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.chrome.browser.fullscreen.FullscreenHtmlApiHandler.FullscreenHtmlApiDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;
import org.chromium.chrome.browser.widget.ControlContainer;
import org.chromium.content.browser.ContentVideoView;
import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.common.BrowserControlsState;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;

/**
 * A class that manages control and content views to create the fullscreen mode.
 */
public class ChromeFullscreenManager
        extends FullscreenManager implements ActivityStateListener, WindowFocusChangedListener {

    // The amount of time to delay the control show request after returning to a once visible
    // activity.  This delay is meant to allow Android to run its Activity focusing animation and
    // have the controls scroll back in smoothly once that has finished.
    private static final long ACTIVITY_RETURN_SHOW_REQUEST_DELAY_MS = 100;

    private final Activity mActivity;
    private final BrowserStateBrowserControlsVisibilityDelegate mBrowserVisibilityDelegate;
    @ControlsPosition private final int mControlsPosition;
    private final boolean mExitFullscreenOnStop;

    private ControlContainer mControlContainer;
    private int mTopControlContainerHeight;
    private int mBottomControlContainerHeight;
    private boolean mControlsResizeView;
    private TabModelSelectorTabModelObserver mTabModelObserver;

    private float mRendererTopControlOffset = Float.NaN;
    private float mRendererBottomControlOffset = Float.NaN;
    private float mRendererTopContentOffset;
    private float mPreviousContentOffset = Float.NaN;
    private float mControlOffsetRatio;
    private float mPreviousControlOffset;
    private boolean mIsEnteringPersistentModeState;

    private boolean mInGesture;
    private boolean mContentViewScrolling;

    private boolean mBrowserControlsAndroidViewHidden;

    private final ArrayList<FullscreenListener> mListeners = new ArrayList<>();

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({CONTROLS_POSITION_TOP, CONTROLS_POSITION_BOTTOM, CONTROLS_POSITION_NONE})
    public @interface ControlsPosition {}

    /** Controls are at the top, eg normal ChromeTabbedActivity. */
    public static final int CONTROLS_POSITION_TOP = 0;
    /** Controls are at the bottom, eg ChromeTabbedActivity with Chrome Home enabled. */
    public static final int CONTROLS_POSITION_BOTTOM = 1;
    /** Controls are not present, eg FullscreenActivity. */
    public static final int CONTROLS_POSITION_NONE = 2;

    /**
     * A listener that gets notified of changes to the fullscreen state.
     */
    public interface FullscreenListener {
        /**
         * Called whenever the content's offset changes.
         * @param offset The new offset of the content from the top of the screen.
         */
        public void onContentOffsetChanged(float offset);

        /**
         * Called whenever the controls' offset changes.
         * @param topOffset    The new value of the offset from the top of the top control.
         * @param bottomOffset The new value of the offset from the top of the bottom control.
         * @param needsAnimate Whether the caller is driving an animation with further updates.
         */
        public void onControlsOffsetChanged(float topOffset, float bottomOffset,
                boolean needsAnimate);

        /**
         * Called when a ContentVideoView is created/destroyed.
         * @param enabled Whether to enter or leave overlay video mode.
         */
        public void onToggleOverlayVideoMode(boolean enabled);

        /**
         * Called when the height of the controls are changed.
         */
        public void onBottomControlsHeightChanged(int bottomControlsHeight);

        /**
         * Called when the viewport size of the active content is updated.
         */
        public default void onUpdateViewportSize() {}
    }

    private final Runnable mUpdateVisibilityRunnable = new Runnable() {
        @Override
        public void run() {
            int visibility = shouldShowAndroidControls() ? View.VISIBLE : View.INVISIBLE;
            if (mControlContainer.getView().getVisibility() == visibility) return;
            // requestLayout is required to trigger a new gatherTransparentRegion(), which
            // only occurs together with a layout and let's SurfaceFlinger trim overlays.
            // This may be almost equivalent to using View.GONE, but we still use View.INVISIBLE
            // since drawing caches etc. won't be destroyed, and the layout may be less expensive.
            mControlContainer.getView().setVisibility(visibility);
            mControlContainer.getView().requestLayout();
        }
    };

    /**
     * Creates an instance of the fullscreen mode manager.
     * @param activity The activity that supports fullscreen.
     * @param controlsPosition Where the browser controls are.
     */
    public ChromeFullscreenManager(Activity activity, @ControlsPosition int controlsPosition) {
        this(activity, controlsPosition, true);
    }

    /**
     * Creates an instance of the fullscreen mode manager.
     * @param activity The activity that supports fullscreen.
     * @param controlsPosition Where the browser controls are.
     * @param exitFullscreenOnStop Whether fullscreen mode should exit on stop - should be
     *                             true for Activities that are not always fullscreen.
     */
    public ChromeFullscreenManager(Activity activity, @ControlsPosition int controlsPosition,
            boolean exitFullscreenOnStop) {
        super(activity.getWindow());

        mActivity = activity;
        mControlsPosition = controlsPosition;
        mExitFullscreenOnStop = exitFullscreenOnStop;
        mBrowserVisibilityDelegate = new BrowserStateBrowserControlsVisibilityDelegate(
                new Runnable() {
                    @Override
                    public void run() {
                        if (getTab() != null) {
                            getTab().updateFullscreenEnabledState();
                        } else if (!mBrowserVisibilityDelegate.canAutoHideBrowserControls()) {
                            setPositionsForTabToNonFullscreen();
                        }
                    }
                });
    }

    /**
     * Initializes the fullscreen manager with the required dependencies.
     *
     * @param controlContainer Container holding the controls (Toolbar).
     * @param modelSelector The tab model selector that will be monitored for tab changes.
     * @param resControlContainerHeight The dimension resource ID for the control container height.
     */
    public void initialize(ControlContainer controlContainer, final TabModelSelector modelSelector,
            int resControlContainerHeight) {
        ApplicationStatus.registerStateListenerForActivity(this, mActivity);
        ApplicationStatus.registerWindowFocusChangedListener(this);

        mTabModelObserver = new TabModelSelectorTabModelObserver(modelSelector) {
            @Override
            public void tabClosureCommitted(Tab tab) {
                setTab(modelSelector.getCurrentTab());
            }

            @Override
            public void allTabsClosureCommitted() {
                setTab(modelSelector.getCurrentTab());
            }

            @Override
            public void tabRemoved(Tab tab) {
                setTab(modelSelector.getCurrentTab());
            }

            @Override
            public void didSelectTab(Tab tab, TabSelectionType type, int lastId) {
                setTab(modelSelector.getCurrentTab());
            }

            @Override
            public void didCloseTab(int tabId, boolean incognito) {
                setTab(modelSelector.getCurrentTab());
            }

            @Override
            public void pendingTabAdd(boolean isPendingTabAdd) {
                setTab(modelSelector.getCurrentTab());
            }
        };

        assert controlContainer != null;
        mControlContainer = controlContainer;

        int controlContainerHeight =
                mActivity.getResources().getDimensionPixelSize(resControlContainerHeight);

        switch (mControlsPosition) {
            case CONTROLS_POSITION_TOP:
                mTopControlContainerHeight = controlContainerHeight;
                break;
            case CONTROLS_POSITION_BOTTOM:
                mBottomControlContainerHeight = controlContainerHeight;
                break;
            case CONTROLS_POSITION_NONE:
                // Treat the case of no controls as controls always being totally offscreen.
                mControlOffsetRatio = 1.0f;
                break;
        }

        mRendererTopContentOffset = mTopControlContainerHeight;
        updateControlOffset();
    }

    @Override
    public boolean areBrowserControlsAtBottom() {
        return mControlsPosition == CONTROLS_POSITION_BOTTOM;
    }

    /**
     * @return The visibility delegate that allows browser UI to control the browser control
     *         visibility.
     */
    public BrowserStateBrowserControlsVisibilityDelegate getBrowserVisibilityDelegate() {
        return mBrowserVisibilityDelegate;
    }

    @Override
    public void setTab(@Nullable Tab tab) {
        Tab previousTab = getTab();
        super.setTab(tab);
        if (tab != null && previousTab != getTab()) {
            mBrowserVisibilityDelegate.showControlsTransient();
        }
        if (tab == null && !mBrowserVisibilityDelegate.canAutoHideBrowserControls()) {
            setPositionsForTabToNonFullscreen();
        }
    }

    @Override
    public void onActivityStateChange(Activity activity, int newState) {
        if (newState == ActivityState.STOPPED && mExitFullscreenOnStop) {
            // Exit fullscreen in onStop to ensure the system UI flags are set correctly when
            // showing again (on JB MR2+ builds, the omnibox would be covered by the
            // notification bar when this was done in onStart()).
            setPersistentFullscreenMode(false);
        } else if (newState == ActivityState.STARTED) {
            ThreadUtils.postOnUiThreadDelayed(new Runnable() {
                @Override
                public void run() {
                    mBrowserVisibilityDelegate.showControlsTransient();
                }
            }, ACTIVITY_RETURN_SHOW_REQUEST_DELAY_MS);
        } else if (newState == ActivityState.DESTROYED) {
            ApplicationStatus.unregisterActivityStateListener(this);
            ApplicationStatus.unregisterWindowFocusChangedListener(this);

            mTabModelObserver.destroy();
        }
    }

    @Override
    public void onWindowFocusChanged(Activity activity, boolean hasFocus) {
        if (mActivity != activity) return;
        onWindowFocusChanged(hasFocus);
        // {@link ContentVideoView#getContentVideoView} requires native to have been initialized.
        if (!LibraryLoader.isInitialized()) return;
        ContentVideoView videoView = ContentVideoView.getContentVideoView();
        if (videoView != null) {
            videoView.onFullscreenWindowFocused();
        }
    }

    @Override
    protected FullscreenHtmlApiDelegate createApiDelegate() {
        return new FullscreenHtmlApiDelegate() {
            @Override
            public void onEnterFullscreen() {
                Tab tab = getTab();
                if (areBrowserControlsOffScreen()) {
                    // The browser controls are currently hidden.
                    getHtmlApiHandler().enterFullscreen(tab);
                } else {
                    // We should hide browser controls first.
                    mIsEnteringPersistentModeState = true;
                    tab.updateFullscreenEnabledState();
                }
            }

            @Override
            public boolean cancelPendingEnterFullscreen() {
                boolean wasPending = mIsEnteringPersistentModeState;
                mIsEnteringPersistentModeState = false;
                return wasPending;
            }

            @Override
            public void onFullscreenExited(Tab tab) {
                // At this point, browser controls are hidden. Show browser controls only if it's
                // permitted.
                tab.updateBrowserControlsState(BrowserControlsState.SHOWN, true);
            }

            @Override
            public boolean shouldShowNotificationToast() {
                return !isOverlayVideoMode();
            }
        };
    }

    @Override
    public float getBrowserControlHiddenRatio() {
        return mControlOffsetRatio;
    }

    /**
     * @return True if the browser controls are completely off screen.
     */
    public boolean areBrowserControlsOffScreen() {
        return getBrowserControlHiddenRatio() == 1.0f;
    }

    /**
     * @return Whether the browser controls should be drawn as a texture.
     */
    public boolean drawControlsAsTexture() {
        return getBrowserControlHiddenRatio() > 0;
    }

    /**
     * Sets the height of the bottom controls.
     */
    public void setBottomControlsHeight(int bottomControlsHeight) {
        if (mBottomControlContainerHeight == bottomControlsHeight) return;
        mBottomControlContainerHeight = bottomControlsHeight;
        for (int i = 0; i < mListeners.size(); i++) {
            mListeners.get(i).onBottomControlsHeightChanged(mBottomControlContainerHeight);
        }
    }

    @Override
    public int getTopControlsHeight() {
        return mTopControlContainerHeight;
    }

    /**
     * @return The height of the bottom controls in pixels.
     */
    public int getBottomControlsHeight() {
        return mBottomControlContainerHeight;
    }

    /**
     * @return The height of the bottom controls in pixels.
     */
    public boolean controlsResizeView() {
        return mControlsResizeView;
    }

    @Override
    public float getContentOffset() {
        return mRendererTopContentOffset;
    }

    /**
     * @return The offset of the controls from the top of the screen.
     */
    public float getTopControlOffset() {
        // This is to avoid a problem with -0f in tests.
        if (mControlOffsetRatio == 0f) return 0f;
        return mControlOffsetRatio * -getTopControlsHeight();
    }

    /**
     * @return The offset of the controls from the bottom of the screen.
     */
    public float getBottomControlOffset() {
        if (mControlOffsetRatio == 0f) return 0f;
        return mControlOffsetRatio * getBottomControlsHeight();

    }

    /**
     * @return The toolbar control container.
     */
    public ControlContainer getControlContainer() {
        return mControlContainer;
    }

    private void updateControlOffset() {
        if (mControlsPosition == CONTROLS_POSITION_NONE) return;

        float topOffsetRatio = 0;

        float rendererControlOffset;
        if (mControlsPosition == CONTROLS_POSITION_BOTTOM) {
            rendererControlOffset =
                    Math.abs(mRendererBottomControlOffset / getBottomControlsHeight());
        } else {
            rendererControlOffset = Math.abs(mRendererTopControlOffset / getTopControlsHeight());
        }

        final boolean isNaNRendererControlOffset = Float.isNaN(rendererControlOffset);
        if (!isNaNRendererControlOffset) topOffsetRatio = rendererControlOffset;
        mControlOffsetRatio = topOffsetRatio;
    }

    @Override
    public void setOverlayVideoMode(boolean enabled) {
        super.setOverlayVideoMode(enabled);

        for (int i = 0; i < mListeners.size(); i++) {
            mListeners.get(i).onToggleOverlayVideoMode(enabled);
        }
    }

    /**
     * @return The visible offset of the content from the top of the screen.
     */
    public float getTopVisibleContentOffset() {
        return getTopControlsHeight() + getTopControlOffset();
    }

    /**
     * @param listener The {@link FullscreenListener} to be notified of fullscreen changes.
     */
    public void addListener(FullscreenListener listener) {
        if (!mListeners.contains(listener)) mListeners.add(listener);
    }

    /**
     * @param listener The {@link FullscreenListener} to no longer be notified of fullscreen
     *                 changes.
     */
    public void removeListener(FullscreenListener listener) {
        mListeners.remove(listener);
    }

    /**
     * Updates viewport size to have it render the content correctly.
     */
    public void updateViewportSize() {
        if (mInGesture || mContentViewScrolling) return;

        // Update content viewport size only when the browser controls are not animating.
        int topContentOffset = (int) mRendererTopContentOffset;
        int bottomControlOffset = (int) mRendererBottomControlOffset;
        if ((topContentOffset != 0 && topContentOffset != getTopControlsHeight())
                && bottomControlOffset != 0 && bottomControlOffset != getBottomControlsHeight()) {
            return;
        }
        boolean controlsResizeView =
                topContentOffset > 0 || bottomControlOffset < getBottomControlsHeight();
        mControlsResizeView = controlsResizeView;
        Tab tab = getTab();
        if (tab == null) return;
        tab.setTopControlsHeight(getTopControlsHeight(), controlsResizeView);
        tab.setBottomControlsHeight(getBottomControlsHeight());
        for (FullscreenListener listener : mListeners) listener.onUpdateViewportSize();
    }

    @Override
    public void updateContentViewChildrenState() {
        ContentViewCore contentViewCore = getActiveContentViewCore();
        if (contentViewCore == null) return;
        ViewGroup view = contentViewCore.getContainerView();

        float topViewsTranslation = getTopVisibleContentOffset();
        float bottomMargin = getBottomControlsHeight() - getBottomControlOffset();
        applyTranslationToTopChildViews(view, topViewsTranslation);
        applyMarginToFullChildViews(view, topViewsTranslation, bottomMargin);
        updateViewportSize();
    }

    /**
     * Utility routine for ensuring visibility updates are synchronized with
     * animation, preventing message loop stalls due to untimely invalidation.
     */
    private void scheduleVisibilityUpdate() {
        final int desiredVisibility = shouldShowAndroidControls() ? View.VISIBLE : View.INVISIBLE;
        if (mControlContainer.getView().getVisibility() == desiredVisibility) return;
        mControlContainer.getView().removeCallbacks(mUpdateVisibilityRunnable);
        mControlContainer.getView().postOnAnimation(mUpdateVisibilityRunnable);
    }

    private void updateVisuals() {
        TraceEvent.begin("FullscreenManager:updateVisuals");

        float offset = 0f;
        if (mControlsPosition == CONTROLS_POSITION_BOTTOM) {
            offset = getBottomControlOffset();
        } else if (mControlsPosition == CONTROLS_POSITION_TOP) {
            offset = getTopControlOffset();
        }

        if (Float.compare(mPreviousControlOffset, offset) != 0) {
            mPreviousControlOffset = offset;

            scheduleVisibilityUpdate();
            if (shouldShowAndroidControls()) {
                mControlContainer.getView().setTranslationY(offset);
            }

            // Whether we need the compositor to draw again to update our animation.
            // Should be |false| when the browser controls are only moved through the page
            // scrolling.
            boolean needsAnimate = shouldShowAndroidControls();
            for (int i = 0; i < mListeners.size(); i++) {
                mListeners.get(i).onControlsOffsetChanged(
                        getTopControlOffset(), getBottomControlOffset(), needsAnimate);
            }
        }

        final Tab tab = getTab();
        if (tab != null && areBrowserControlsOffScreen() && mIsEnteringPersistentModeState) {
            getHtmlApiHandler().enterFullscreen(tab);
            mIsEnteringPersistentModeState = false;
        }

        updateContentViewChildrenState();

        float contentOffset = getContentOffset();
        if (Float.compare(mPreviousContentOffset, contentOffset) != 0) {
            for (int i = 0; i < mListeners.size(); i++) {
                mListeners.get(i).onContentOffsetChanged(contentOffset);
            }
            mPreviousContentOffset = contentOffset;
        }

        TraceEvent.end("FullscreenManager:updateVisuals");
    }

    /**
     * @param hide Whether or not to force the browser controls Android view to hide.  If this is
     *             {@code false} the browser controls Android view will show/hide based on position,
     *             if it is {@code true} the browser controls Android view will always be hidden.
     */
    public void setHideBrowserControlsAndroidView(boolean hide) {
        if (mBrowserControlsAndroidViewHidden == hide) return;
        mBrowserControlsAndroidViewHidden = hide;
        scheduleVisibilityUpdate();
    }

    private boolean shouldShowAndroidControls() {
        if (mBrowserControlsAndroidViewHidden) return false;

        boolean showControls = !drawControlsAsTexture();
        ContentViewCore contentViewCore = getActiveContentViewCore();
        if (contentViewCore == null) return showControls;
        ViewGroup contentView = contentViewCore.getContainerView();

        for (int i = 0; i < contentView.getChildCount(); i++) {
            View child = contentView.getChildAt(i);
            if (!(child.getLayoutParams() instanceof FrameLayout.LayoutParams)) continue;

            FrameLayout.LayoutParams layoutParams =
                    (FrameLayout.LayoutParams) child.getLayoutParams();
            if (Gravity.TOP == (layoutParams.gravity & Gravity.FILL_VERTICAL)) {
                showControls = true;
                break;
            }
        }

        return showControls;
    }

    private void applyMarginToFullChildViews(
            ViewGroup contentView, float topMargin, float bottomMargin) {
        for (int i = 0; i < contentView.getChildCount(); i++) {
            View child = contentView.getChildAt(i);
            if (!(child.getLayoutParams() instanceof FrameLayout.LayoutParams)) continue;
            FrameLayout.LayoutParams layoutParams =
                    (FrameLayout.LayoutParams) child.getLayoutParams();

            if (layoutParams.height == LayoutParams.MATCH_PARENT
                    && (layoutParams.topMargin != (int) topMargin
                               || layoutParams.bottomMargin != (int) bottomMargin)) {
                layoutParams.topMargin = (int) topMargin;
                layoutParams.bottomMargin = (int) bottomMargin;
                child.requestLayout();
                TraceEvent.instant("FullscreenManager:child.requestLayout()");
            }
        }
    }

    private void applyTranslationToTopChildViews(ViewGroup contentView, float translation) {
        for (int i = 0; i < contentView.getChildCount(); i++) {
            View child = contentView.getChildAt(i);
            if (!(child.getLayoutParams() instanceof FrameLayout.LayoutParams)) continue;

            FrameLayout.LayoutParams layoutParams =
                    (FrameLayout.LayoutParams) child.getLayoutParams();
            if (Gravity.TOP == (layoutParams.gravity & Gravity.FILL_VERTICAL)) {
                child.setTranslationY(translation);
                TraceEvent.instant("FullscreenManager:child.setTranslationY()");
            }
        }
    }

    private ContentViewCore getActiveContentViewCore() {
        Tab tab = getTab();
        return tab != null ? tab.getContentViewCore() : null;
    }

    @Override
    public void setPositionsForTabToNonFullscreen() {
        Tab tab = getTab();
        if (tab == null || tab.canShowBrowserControls()) {
            setPositionsForTab(0, 0, getTopControlsHeight());
        } else {
            setPositionsForTab(-getTopControlsHeight(), getBottomControlsHeight(), 0);
        }
    }

    @Override
    public void setPositionsForTab(float topControlsOffset, float bottomControlsOffset,
            float topContentOffset) {
        if (VrShellDelegate.isInVr()) {
            // The dip scale of java UI and WebContents are different while in VR, leading to a
            // mismatch in size in pixels when converting from dips. Since we hide the controls in
            // VR anyways, just set the offsets to what they're supposed to be with the controls
            // hidden.
            // TODO(mthiesse): Should we instead just set the top controls height to be 0 while in
            // VR?
            topControlsOffset = -getTopControlsHeight();
            bottomControlsOffset = getBottomControlsHeight();
            topContentOffset = 0;
        }
        float rendererTopControlOffset =
                Math.round(Math.max(topControlsOffset, -getTopControlsHeight()));
        float rendererBottomControlOffset =
                Math.round(Math.min(bottomControlsOffset, getBottomControlsHeight()));

        float rendererTopContentOffset = Math.min(
                Math.round(topContentOffset), rendererTopControlOffset + getTopControlsHeight());

        if (Float.compare(rendererTopControlOffset, mRendererTopControlOffset) == 0
                && Float.compare(rendererBottomControlOffset, mRendererBottomControlOffset) == 0
                && Float.compare(rendererTopContentOffset, mRendererTopContentOffset) == 0) {
            return;
        }

        mRendererTopControlOffset = rendererTopControlOffset;
        mRendererBottomControlOffset = rendererBottomControlOffset;

        mRendererTopContentOffset = rendererTopContentOffset;
        updateControlOffset();

        updateVisuals();
    }

    /**
     * Notifies the fullscreen manager that a motion event has occurred.
     * @param e The dispatched motion event.
     */
    public void onMotionEvent(MotionEvent e) {
        int eventAction = e.getActionMasked();
        if (eventAction == MotionEvent.ACTION_DOWN
                || eventAction == MotionEvent.ACTION_POINTER_DOWN) {
            mInGesture = true;
            // TODO(qinmin): Probably there is no need to hide the toast as it will go away
            // by itself.
            getHtmlApiHandler().hideNotificationToast();
        } else if (eventAction == MotionEvent.ACTION_CANCEL
                || eventAction == MotionEvent.ACTION_UP) {
            mInGesture = false;
            updateVisuals();
        }
    }

    @Override
    public void onContentViewScrollingStateChanged(boolean scrolling) {
        mContentViewScrolling = scrolling;
        if (!scrolling) updateVisuals();
    }
}
