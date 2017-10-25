// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.content.res.Resources;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;

import org.chromium.base.CollectionUtil;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.ContextMenuManager.TouchEnabledDelegate;
import org.chromium.chrome.browser.ntp.LogoBridge.Logo;
import org.chromium.chrome.browser.ntp.LogoBridge.LogoObserver;
import org.chromium.chrome.browser.ntp.LogoDelegateImpl;
import org.chromium.chrome.browser.ntp.LogoView;
import org.chromium.chrome.browser.ntp.cards.NewTabPageAdapter;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.omnibox.LocationBarPhone;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.search_engines.TemplateUrlService.TemplateUrlServiceObserver;
import org.chromium.chrome.browser.snackbar.SnackbarManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.widget.animation.CancelAwareAnimatorListener;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.StateChangeReason;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetContentController;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetNewTabController;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

import java.util.List;

/**
 * Provides content to be displayed inside of the Home tab of bottom sheet.
 */
public class SuggestionsBottomSheetContent implements BottomSheet.BottomSheetContent,
                                                      BottomSheetNewTabController.Observer,
                                                      TemplateUrlServiceObserver {
    private final View mView;
    private final SuggestionsRecyclerView mRecyclerView;
    private final ContextMenuManager mContextMenuManager;
    private final SuggestionsUiDelegateImpl mSuggestionsUiDelegate;
    private final TileGroup.Delegate mTileGroupDelegate;
    @Nullable
    private final SuggestionsCarousel mSuggestionsCarousel;
    private final SuggestionsSheetVisibilityChangeObserver mBottomSheetObserver;
    private final ChromeActivity mActivity;
    private final LocationBarPhone mLocationBar;
    private final BottomSheet mSheet;
    private final LogoView mLogoView;
    private final LogoDelegateImpl mLogoDelegate;
    private final ViewGroup mControlContainerView;
    private final View mToolbarPullHandle;
    private final View mToolbarShadow;

    private boolean mNewTabShown;
    private boolean mSearchProviderHasLogo = true;
    private float mLastSheetHeightFraction = 1f;
    private final int mLogoHeight;
    private final int mToolbarHeight;
//    private final float mMaxToolbarOffset;
    private int mScrollY;
    private ValueAnimator mTransitionAnimator;
    private final Interpolator mTransitionInterpolator = new DecelerateInterpolator(1.0f);

    /**
     * Whether {@code mView} is currently attached to the window. This is used in place of
     * {@link View#isAttachedToWindow()} to support older versions of Android (KitKat & JellyBean).
     */
    private boolean mIsAttachedToWindow;

    public SuggestionsBottomSheetContent(final ChromeActivity activity, final BottomSheet sheet,
            TabModelSelector tabModelSelector, SnackbarManager snackbarManager) {
        SuggestionsDependencyFactory depsFactory = SuggestionsDependencyFactory.getInstance();
        Profile profile = Profile.getLastUsedProfile();
        SuggestionsNavigationDelegate navigationDelegate =
                new SuggestionsNavigationDelegateImpl(activity, profile, sheet, tabModelSelector);
        mActivity = activity;
        mSheet = sheet;
        mTileGroupDelegate =
                new TileGroupDelegateImpl(activity, profile, navigationDelegate, snackbarManager);
        mSuggestionsUiDelegate = new SuggestionsUiDelegateImpl(
                depsFactory.createSuggestionSource(profile), depsFactory.createEventReporter(),
                navigationDelegate, profile, sheet, activity.getReferencePool(), snackbarManager);

        mView = LayoutInflater.from(activity).inflate(
                R.layout.suggestions_bottom_sheet_content, null);
        Resources resources = mView.getResources();
        int backgroundColor = SuggestionsConfig.getBackgroundColor(resources);
        mView.setBackgroundColor(backgroundColor);
        mRecyclerView = mView.findViewById(R.id.recycler_view);
        mRecyclerView.setBackgroundColor(backgroundColor);
//        mRecyclerView.setBackgroundColor(0xffffcccc);
        mLogoHeight = resources.getDimensionPixelSize(R.dimen.ntp_logo_height_modern);
        mToolbarHeight = resources.getDimensionPixelSize(R.dimen.bottom_control_container_height);
//        mMaxToolbarOffset =
//                resources.getDimensionPixelSize(R.dimen.ntp_logo_height_modern) +
//                        resources.getDimensionPixelSize(R.dimen.ntp_logo_margin_top_modern) +
//                        resources.getDimensionPixelSize(R.dimen.ntp_logo_margin_bottom_modern);

        TouchEnabledDelegate touchEnabledDelegate = activity.getBottomSheet()::setTouchEnabled;
        mContextMenuManager = new ContextMenuManager(
                navigationDelegate, touchEnabledDelegate, activity::closeContextMenu);
        activity.getWindowAndroid().addContextMenuCloseListener(mContextMenuManager);
        mSuggestionsUiDelegate.addDestructionObserver(() -> {
            activity.getWindowAndroid().removeContextMenuCloseListener(mContextMenuManager);
        });

        UiConfig uiConfig = new UiConfig(mRecyclerView);
        mRecyclerView.init(uiConfig, mContextMenuManager);

        OfflinePageBridge offlinePageBridge = OfflinePageBridge.getForProfile(profile);

        mSuggestionsCarousel =
                ChromeFeatureList.isEnabled(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_CAROUSEL)
                ? new SuggestionsCarousel(
                          uiConfig, mSuggestionsUiDelegate, mContextMenuManager, offlinePageBridge)
                : null;

        final NewTabPageAdapter adapter = new NewTabPageAdapter(mSuggestionsUiDelegate,
                /* aboveTheFoldView = */ null, uiConfig, offlinePageBridge, mContextMenuManager,
                mTileGroupDelegate, mSuggestionsCarousel);

        mBottomSheetObserver = new SuggestionsSheetVisibilityChangeObserver(this, activity) {
            @Override
            public void onContentShown(boolean isFirstShown) {
                // TODO(dgn): Temporary workaround to trigger an event in the backend when the
                // sheet is opened following inactivity. See https://crbug.com/760974. Should be
                // moved back to the "new opening of the sheet" path once we are able to trigger it
                // in that case.
                mSuggestionsUiDelegate.getEventReporter().onSurfaceOpened();
                SuggestionsMetrics.recordSurfaceVisible();

                if (isFirstShown) {
                    adapter.refreshSuggestions();

                    maybeUpdateContextualSuggestions();

                    // Set the adapter on the RecyclerView after updating it, to avoid sending
                    // notifications that might confuse its internal state.
                    // See https://crbug.com/756514.
                    mRecyclerView.setAdapter(adapter);
                    mRecyclerView.scrollToPosition(0);
                    mRecyclerView.getScrollEventReporter().reset();
                }
            }

            @Override
            public void onContentHidden() {
                SuggestionsMetrics.recordSurfaceHidden();
            }

            @Override
            public void onContentStateChanged(@BottomSheet.SheetState int contentState) {
                if (contentState == BottomSheet.SHEET_STATE_HALF) {
                    SuggestionsMetrics.recordSurfaceHalfVisible();
                    mRecyclerView.setScrollEnabled(false);
                } else if (contentState == BottomSheet.SHEET_STATE_FULL) {
                    SuggestionsMetrics.recordSurfaceFullyVisible();
                    mRecyclerView.setScrollEnabled(true);
                }

                Log.w("mvano", "onContentStateChanged calling updateLogoTransition");
                updateLogoTransition();
            }

            @Override
            public void onSheetClosed(@StateChangeReason int reason) {
                super.onSheetClosed(reason);
                mRecyclerView.setAdapter(null);
                Log.w("mvano", "onSheetClosed calling updateLogoTransition");
                updateLogoTransition();
            }

            @Override
            public void onSheetOffsetChanged(float heightFraction) {
                mLastSheetHeightFraction = heightFraction;
                Log.w("mvano", "onSheetOffsetChanged calling updateLogoTransition");
                updateLogoTransition();
            }
        };

        mLocationBar = sheet.findViewById(R.id.location_bar);
        mRecyclerView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (mLocationBar != null && mLocationBar.isUrlBarFocused()) {
                    mLocationBar.setUrlBarFocus(false);
                }

                // Never intercept the touch event.
                return false;
            }
        });

        // TODO: do we risk memory leaks now? Do they all have to be removed again? When?
        mLocationBar.addUrlFocusChangeListener(hasFocus -> {
            Log.w("mvano", "url focus change hasFocus: %s", hasFocus);
            startTransitionAnimation(hasFocus);
        });

        mLogoView = mView.findViewById(R.id.search_provider_logo);
        mControlContainerView = (ViewGroup) activity.findViewById(R.id.control_container);
        mToolbarPullHandle = activity.findViewById(R.id.toolbar_handle);
        mToolbarShadow = activity.findViewById(R.id.bottom_toolbar_shadow);
        mLogoDelegate = new LogoDelegateImpl(navigationDelegate, mLogoView, profile);
        updateSearchProviderHasLogo();
        if (mSearchProviderHasLogo) {
            mLogoView.showSearchProviderInitialView();
            loadSearchProviderLogo();
        }
        TemplateUrlService.getInstance().addObserver(this);
        sheet.getNewTabController().addObserver(this);

        mView.addOnAttachStateChangeListener(new OnAttachStateChangeListener() {
            @Override
            public void onViewAttachedToWindow(View v) {
                mIsAttachedToWindow = true;
                Log.w("mvano", "onViewAttachedToWindow calling updateLogoTransition");
                updateLogoTransition();
            }

            @Override
            public void onViewDetachedFromWindow(View v) {
                mIsAttachedToWindow = false;
                Log.w("mvano", "onViewDetachedFromWindow calling updateLogoTransition");
                updateLogoTransition();
            }
        });

        mRecyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
            @Override
            public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
                mScrollY += dy;
                Log.w("mvano", "onScrolled calling updateLogoTransition");
                updateLogoTransition();
            }
        });
    }

    @Override
    public View getContentView() {
        return mView;
    }

    @Override
    public List<View> getViewsForPadding() {
        return CollectionUtil.newArrayList(mRecyclerView);
    }

    @Override
    public View getToolbarView() {
        return null;
    }

    @Override
    public boolean isUsingLightToolbarTheme() {
        return false;
    }

    @Override
    public boolean isIncognitoThemedContent() {
        return false;
    }

    @Override
    public int getVerticalScrollOffset() {
        return mRecyclerView.computeVerticalScrollOffset();
    }

    @Override
    public void destroy() {
        mBottomSheetObserver.onDestroy();
        mSuggestionsUiDelegate.onDestroy();
        mTileGroupDelegate.destroy();
        TemplateUrlService.getInstance().removeObserver(this);
        mSheet.getNewTabController().removeObserver(this);
    }

    @Override
    public int getType() {
        return BottomSheetContentController.TYPE_SUGGESTIONS;
    }

    @Override
    public boolean applyDefaultTopPadding() {
        return false;
    }

    @Override
    public void onNewTabShown() {
        mNewTabShown = true;
        updateSpacing();
    }

    @Override
    public void onNewTabHidden() {
        mNewTabShown = false;
        updateSpacing();
    }

    @Override
    public void onTemplateURLServiceChanged() {
        updateSearchProviderHasLogo();
        loadSearchProviderLogo();
        Log.w("mvano", "onTemplateURLServiceChanged calling updateLogoTransition");
        updateLogoTransition();
    }

    private void maybeUpdateContextualSuggestions() {
        if (mSuggestionsCarousel == null) return;
        assert ChromeFeatureList.isEnabled(ChromeFeatureList.CONTEXTUAL_SUGGESTIONS_CAROUSEL);

        Tab activeTab = mSheet.getActiveTab();
        final String currentUrl = activeTab == null ? null : activeTab.getUrl();

        mSuggestionsCarousel.refresh(mSheet.getContext(), currentUrl);
    }

    private void updateSearchProviderHasLogo() {
        mSearchProviderHasLogo = TemplateUrlService.getInstance().doesDefaultSearchEngineHaveLogo();
    }

    /**
     * Loads the search provider logo, if any.
     */
    private void loadSearchProviderLogo() {
        if (!mSearchProviderHasLogo) return;

        mLogoView.showSearchProviderInitialView();

        mLogoDelegate.getSearchProviderLogo(new LogoObserver() {
            @Override
            public void onLogoAvailable(Logo logo, boolean fromCache) {
                if (logo == null && fromCache) return;

                mLogoView.setDelegate(mLogoDelegate);
                mLogoView.updateLogo(logo);
            }
        });
    }

    private void startTransitionAnimation(final boolean toolbarHasFocus) {
        if (mTransitionAnimator != null) mTransitionAnimator.cancel();
        float scrollBasedFraction = Math.min(1, mScrollY / getMaxToolbarOffset());
        float startValue = toolbarHasFocus ? scrollBasedFraction : 1.0f;
        float endValue = toolbarHasFocus ? 1.0f : scrollBasedFraction;
        mTransitionAnimator = ValueAnimator.ofFloat(startValue, endValue);
        mTransitionAnimator.setDuration(150L);
        mTransitionAnimator.setInterpolator(mTransitionInterpolator);
        mTransitionAnimator.addUpdateListener(animator -> updateLogoTransition());
        mTransitionAnimator.addListener(new CancelAwareAnimatorListener() {
            @Override
            public void onStart(Animator animator) {
                Log.w("mvano", "animation onStart");
                mLocationBar.finishUrlFocusChange(toolbarHasFocus);
            }
            @Override
            public void onCancel(Animator animator) {
                Log.w("mvano", "animation onCancel");
                mLocationBar.finishUrlFocusChange(toolbarHasFocus);
            }
            @Override
            public void onEnd(Animator animator) {
                Log.w("mvano", "animation onEnd");
                mLocationBar.finishUrlFocusChange(toolbarHasFocus);
            }
        });
        mTransitionAnimator.start();
        // TODO: null out the animator when it is cancelled or ended?
        // TODO: cancel and null out animation in destroy()?
    }

    private void updateLogoTransition() {
        boolean showLogo = shouldShowLogo();
        Log.w("mvano", "updateLogoTransition showLogo: %s", showLogo);
        Log.w("mvano", "updateLogoTransition mScrollY: %s", mScrollY);

        // If the logo is not shown, reset all transitions.
        if (!showLogo) {
            mLogoView.setVisibility(View.GONE);
            mControlContainerView.setTranslationY(0);
            mToolbarPullHandle.setTranslationY(0);
            mToolbarShadow.setTranslationY(0);
            mRecyclerView.setTranslationY(0);
            ViewUtils.setAncestorsShouldClipChildren(mControlContainerView, true);
            return;
        }

        mLogoView.setVisibility(View.VISIBLE);
        ViewUtils.setAncestorsShouldClipChildren(mControlContainerView, false);

        // Transform the sheet height fraction back to pixel scale.
        float rangePx =
                (mSheet.getFullRatio() - mSheet.getPeekRatio()) * mSheet.getSheetContainerHeight();
        float sheetHeightPx = mLastSheetHeightFraction * rangePx;

        // Calculate the transition fraction for hiding the logo: 0 means the logo is fully visible,
        // 1 means it is fully invisible.
        // The transition starts when the sheet is `2 * maxToolbarOffset` from the bottom or the
        // top. This makes the toolbar stay vertically centered in the sheet during the
        // bottom transition.
//        float transitionFraction =
//                Math.max(Math.max(1 - sheetHeightPx / (2 * mMaxToolbarOffset),
//                                 1 + (sheetHeightPx - rangePx) / (2 * mMaxToolbarOffset)),
//                        0);

        Log.w("mvano", "updateLogoTransition sheetContainerHeight: %s", mSheet.getSheetContainerHeight());
        float maxOffset = getMaxToolbarOffset();
        Log.w("mvano", "updateLogoTransition maxOffset: %s", maxOffset);

        final float transitionFraction;
        boolean isAnimating = mTransitionAnimator != null && mTransitionAnimator.isRunning();
        Log.w("mvano", "updateLogoTransition isAnimating: %s", isAnimating);
        Log.w("mvano", "updateLogoTransition mLocationBar.hasFocus(): %s", mLocationBar.hasFocus());
        if (isAnimating) {
            transitionFraction = (float) mTransitionAnimator.getAnimatedValue();
        } else if (mLocationBar.hasFocus()) {
            transitionFraction = 1.0f;
        } else {
            transitionFraction = Math.min(1, mScrollY / maxOffset);
        }
        Log.w("mvano", "updateLogoTransition transitionFraction: %s", transitionFraction);

        ViewGroup.MarginLayoutParams logoParams =
                (ViewGroup.MarginLayoutParams) mLogoView.getLayoutParams();
        mLogoView.setTranslationY(logoParams.topMargin * -transitionFraction);
        mLogoView.setAlpha(Math.max(0.0f, 1.0f - (transitionFraction * 3.0f)));

        float toolbarOffset = maxOffset * (1.0f - transitionFraction);
        mControlContainerView.setTranslationY(toolbarOffset);
        mToolbarPullHandle.setTranslationY(-toolbarOffset);
        mToolbarShadow.setTranslationY(-toolbarOffset);
//        float recyclerViewOffset = mLocationBar.hasFocus() ? toolbarOffset - maxOffset : 0.0f;
//        mRecyclerView.setTranslationY(recyclerViewOffset);
        Log.w("mvano", "========== updateLogoTransition end ==========");
    }

    @Override
    public void scrollToTop() {
        mRecyclerView.smoothScrollToPosition(0);
    }

    private void updateSpacing() {
        int newPadding = mToolbarHeight;
        float maxOffset = getMaxToolbarOffset();
        Log.w("mvano", "updateSpacing maxOffset: %s", maxOffset);
        if (shouldShowLogo()) newPadding += (int) maxOffset;
        Log.w("mvano", "setRecyclerViewTopPadding: newPadding %s", newPadding);
        int currentPadding = mRecyclerView.getPaddingTop();
        if (newPadding == currentPadding) return;

        int left = mRecyclerView.getPaddingLeft();
        int right = mRecyclerView.getPaddingRight();
        int bottom = mRecyclerView.getPaddingBottom();
        mRecyclerView.setPadding(left, newPadding, right, bottom);

        ViewGroup.MarginLayoutParams logoParams =
                (ViewGroup.MarginLayoutParams) mLogoView.getLayoutParams();
        int margin = (int) (maxOffset - mLogoHeight) / 2;
        logoParams.topMargin = margin;
        logoParams.bottomMargin = margin;
        mLogoView.setLayoutParams(logoParams);

        mRecyclerView.scrollToPosition(0);
    }

    private float getMaxToolbarOffset() {
        return (mSheet.getSheetContainerHeight() - mToolbarHeight) * mSheet.getHalfRatio();
    }

    private boolean shouldShowLogo() {
        return mSearchProviderHasLogo && mNewTabShown && mSheet.isSheetOpen()
                && !mActivity.getTabModelSelector().isIncognitoSelected() && mIsAttachedToWindow;
    }
}
