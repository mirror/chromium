// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.annotation.SuppressLint;
import android.content.res.Resources;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.view.ViewGroup;

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
import org.chromium.chrome.browser.omnibox.LocationBar;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.search_engines.TemplateUrlService.TemplateUrlServiceObserver;
import org.chromium.chrome.browser.snackbar.SnackbarManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.StateChangeReason;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetContentController;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetNewTabController;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

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
    private final BottomSheet mSheet;
    private final LogoView mLogoView;
    private final LogoDelegateImpl mLogoDelegate;
    private final ViewGroup mControlContainerView;
    private final View mToolbarPullHandle;
    private final View mToolbarShadow;

    private boolean mNewTabShown;
    private boolean mSearchProviderHasLogo = true;
    private float mLastSheetHeightFraction = 1f;
    private final int mToolbarHeight;
    private final float mMaxToolbarOffset;
    private int mScrollY;

    /**
     * Whether {@code mView} is currently attached to the window. This is used in place of
     * {@link View#isAttachedToWindow()} to support older versions of Android (KitKat & JellyBean).
     */
    private boolean mIsAttachedToWindow;

    // The sheet height fractions at which the logo transitions start and end.
    private static final float LOGO_TRANSITION_BOTTOM_START = 0.3f;
    private static final float LOGO_TRANSITION_BOTTOM_END = 0.1f;
    private static final float LOGO_TRANSITION_TOP_START = 0.75f;
    private static final float LOGO_TRANSITION_TOP_END = 0.95f;

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
        mToolbarHeight = resources.getDimensionPixelSize(R.dimen.bottom_control_container_height);
        mMaxToolbarOffset =
                resources.getDimensionPixelSize(R.dimen.ntp_logo_height_modern) +
                resources.getDimensionPixelSize(R.dimen.ntp_logo_margin_top_modern) +
                resources.getDimensionPixelSize(R.dimen.ntp_logo_margin_bottom_modern);

        TouchEnabledDelegate touchEnabledDelegate = new TouchEnabledDelegate() {
            @Override
            public void setTouchEnabled(boolean enabled) {
                activity.getBottomSheet().setTouchEnabled(enabled);
            }
        };

        mContextMenuManager =
                new ContextMenuManager(activity, navigationDelegate, touchEnabledDelegate);
        activity.getWindowAndroid().addContextMenuCloseListener(mContextMenuManager);
        mSuggestionsUiDelegate.addDestructionObserver(new DestructionObserver() {
            @Override
            public void onDestroy() {
                activity.getWindowAndroid().removeContextMenuCloseListener(mContextMenuManager);
            }
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

                Log.w("mvano", "onContentStateChanged");
                updateLogoTransition();
            }

            @Override
            public void onSheetClosed(@StateChangeReason int reason) {
                super.onSheetClosed(reason);
                mRecyclerView.setAdapter(null);
                Log.w("mvano", "onSheetClosed");
                updateLogoTransition();
            }

            @Override
            public void onSheetOffsetChanged(float heightFraction) {
                mLastSheetHeightFraction = heightFraction;
                Log.w("mvano", "onSheetOffsetChanged");
                updateLogoTransition();
            }
        };

        final LocationBar locationBar = sheet.findViewById(R.id.location_bar);
        mRecyclerView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (locationBar != null && locationBar.isUrlBarFocused()) {
                    locationBar.setUrlBarFocus(false);
                }

                // Never intercept the touch event.
                return false;
            }
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
                Log.w("mvano", "onViewAttachedToWindow");
                updateLogoTransition();
            }

            @Override
            public void onViewDetachedFromWindow(View v) {
                mIsAttachedToWindow = false;
                Log.w("mvano", "onViewDetachedFromWindow");
                updateLogoTransition();
            }
        });

        mRecyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
            @Override
            public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
                Log.w("mvano", "onScrolled");
                mScrollY += dy;
                updateLogoTransition();
            }
        });
    }

    @Override
    public View getContentView() {
        return mView;
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
        updateRecyclerViewTopPadding();
    }

    @Override
    public void onNewTabHidden() {
        mNewTabShown = false;
        updateRecyclerViewTopPadding();
    }

    @Override
    public void onTemplateURLServiceChanged() {
        updateSearchProviderHasLogo();
        loadSearchProviderLogo();
        Log.w("mvano", "onTemplateURLServiceChanged");
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
     * Loads the search provider logo (e.g. Google doodle), if any.
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

    private void updateLogoTransition() {
        boolean showLogo = shouldShowLogo();
        Log.w("mvano", "updateLogoTransition showLogo: %s", showLogo);
        Log.w("mvano", "updateLogoTransition isSmallScreen: %s", mSheet.isSmallScreen());

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

        // The logo is visible at 0.0f and hidden at 1.0f.
        // The toolbar is offset maximum at 0.0f and not at all at 1.0f.
        // TODO: invert these values? It seems confusing and inverted everywhere it is used.
        final float transitionFraction;

//        Log.w("mvano", "updateLogoTransition maxToolbarOffset: %s", mMaxToolbarOffset);
//        Log.w("mvano", "updateLogoTransition mLastSheetHeightFraction: %s", mLastSheetHeightFraction);
//        Log.w("mvano", "updateLogoTransition isFirstItemVisible: %s", mRecyclerView.isFirstItemVisible());

        int scrollY = mRecyclerView.computeVerticalScrollOffset();
        Log.w("mvano", "updateLogoTransition scrollY: %s", scrollY);
//        Log.w("mvano", "updateLogoTransition mScrollY: %s", mScrollY);

        boolean translateRecyclerView = false;

        if (mLastSheetHeightFraction > LOGO_TRANSITION_TOP_START) {
            // In the top transition region, the logo disappears as the sheet reaches the top.
//            float range = LOGO_TRANSITION_TOP_END - LOGO_TRANSITION_TOP_START;
//            transitionFraction =
//                    Math.min((mLastSheetHeightFraction - LOGO_TRANSITION_TOP_START) / range, 1.0f);


//          if (mSheet.isSmallScreen()) // TODO enable this condition, or delete it?

            if (scrollY != 0) {
                transitionFraction = Math.min(scrollY / mMaxToolbarOffset, 1.0f);
            } else {
                transitionFraction = 0.0f;
            }
        } else if (mLastSheetHeightFraction < LOGO_TRANSITION_BOTTOM_START) {
            // In the bottom transition region, the logo appears as the sheet opens and vice versa.
            // If the content was scrolled, the logo is not shown.
            if (scrollY != 0) {
                transitionFraction = 1.0f;
            } else {
                float range = LOGO_TRANSITION_BOTTOM_START - LOGO_TRANSITION_BOTTOM_END;
                transitionFraction = Math.min(
                        (LOGO_TRANSITION_BOTTOM_START - mLastSheetHeightFraction) / range, 1.0f);
                translateRecyclerView = true;
            }
        } else {
            // Between the transition regions, the logo is visible if the content was not scrolled.
            if (scrollY != 0) {
                transitionFraction = 1.0f;
            } else {
                transitionFraction = 0.0f;
            }
        }
        Log.w("mvano", "updateLogoTransition translateRecyclerView: %s", translateRecyclerView);
        Log.w("mvano", "updateLogoTransition transitionFraction: %s", transitionFraction);

        // An additional effect that might look nice but might also add too much visual noise.
//        mLogoView.setTranslationY(logoParams.topMargin * -transitionFraction);

        mLogoView.setAlpha(Math.max(0.0f, 1.0f - (transitionFraction * 3.0f)));
        Log.w("mvano", "updateLogoTransition logo alpha: %s", mLogoView.getAlpha());

        float toolbarOffset = mMaxToolbarOffset * (1.0f - transitionFraction);
        mControlContainerView.setTranslationY(toolbarOffset);
        mToolbarPullHandle.setTranslationY(-toolbarOffset);
        mToolbarShadow.setTranslationY(-toolbarOffset);

        // TODO: setting top padding seems to confuse scrollY calculation, resetting it to 0 every other time.
        // It also does not seem to affect the scroll position so it only becomes clear after scrolling to the top that there is extra padding :-/
//        setRecyclerViewTopPadding((int) mMaxToolbarOffset + mToolbarHeight);

        if (translateRecyclerView) mRecyclerView.setTranslationY(toolbarOffset - mMaxToolbarOffset);
        else mRecyclerView.setTranslationY(0);
//        if (scrollY == 0) {
//            setRecyclerViewBottomPadding((int) toolbarOffset);
//            mRecyclerView.setTranslationY(toolbarOffset);
//        }
    }

    @Override
    public void scrollToTop() {
        mRecyclerView.smoothScrollToPosition(0);
    }

    private void updateRecyclerViewTopPadding() {
        int newPadding = mToolbarHeight;
        if (shouldShowLogo()) newPadding += (int) mMaxToolbarOffset;
        Log.w("mvano", "setRecyclerViewTopPadding: newPadding %s", newPadding);
        int currentPadding = mRecyclerView.getPaddingTop();
        if (newPadding == currentPadding) return;

        int left = mRecyclerView.getPaddingLeft();
        int right = mRecyclerView.getPaddingRight();
        int bottom = mRecyclerView.getPaddingBottom();
        mRecyclerView.setPadding(left, newPadding, right, bottom);
        mRecyclerView.scrollToPosition(0);
    }

    private void setRecyclerViewBottomPadding(int newPadding) {
        Log.w("mvano", "setRecyclerViewBottomPadding: newPadding %s", newPadding);
        int currentPadding = mRecyclerView.getPaddingBottom();
        if (newPadding == currentPadding) return;

        int left = mRecyclerView.getPaddingLeft();
        int top = mRecyclerView.getPaddingTop();
        int right = mRecyclerView.getPaddingRight();
        mRecyclerView.setPadding(left, top, right, newPadding);
    }

    private boolean shouldShowLogo() {
        return mSearchProviderHasLogo && mNewTabShown && mSheet.isSheetOpen()
                && !mActivity.getTabModelSelector().isIncognitoSelected() && mIsAttachedToWindow
                && FeatureUtilities.isChromeHomeDoodleEnabled();
    }
}
