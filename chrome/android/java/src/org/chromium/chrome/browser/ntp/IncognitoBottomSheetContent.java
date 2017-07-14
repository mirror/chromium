// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.app.Activity;
import android.view.View;
import android.view.ViewTreeObserver.OnScrollChangedListener;
import android.widget.ScrollView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.FadingShadow;
import org.chromium.chrome.browser.widget.FadingShadowView;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.BottomSheetContent;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetContentController;

/**
 * Provides content to be displayed inside the Home tab of the bottom sheet in incognito mode.
 */
public class IncognitoBottomSheetContent extends IncognitoNewTabPage implements BottomSheetContent {
    private final ScrollView mScrollView;

    /**
     * Constructs a new IncognitoBottomSheetContent.
     * @param activity The {@link Activity} displaying this bottom sheet content.
     */
    public IncognitoBottomSheetContent(final Activity activity) {
        super(activity);

        final FadingShadowView shadow =
                (FadingShadowView) mIncognitoNewTabPageView.findViewById(R.id.shadow);
        shadow.init(ApiCompatibilityUtils.getColor(
                            mIncognitoNewTabPageView.getResources(), R.color.toolbar_shadow_color),
                FadingShadow.POSITION_TOP);

        mScrollView = (ScrollView) mIncognitoNewTabPageView.findViewById(R.id.ntp_scrollview);
        mScrollView.getViewTreeObserver().addOnScrollChangedListener(new OnScrollChangedListener() {
            @Override
            public void onScrollChanged() {
                boolean shadowVisible = mScrollView.canScrollVertically(-1);
                shadow.setVisibility(shadowVisible ? View.VISIBLE : View.GONE);
            }
        });
    }

    @Override
    protected int getLayoutResource() {
        return useMDIncognitoNTP() ? R.layout.incognito_bottom_sheet_content_md
                                   : R.layout.incognito_bottom_sheet_content;
    }

    @Override
    public View getContentView() {
        return getView();
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
        return true;
    }

    @Override
    public int getVerticalScrollOffset() {
        return mScrollView.getScrollY();
    }

    @Override
    public void destroy() {}

    @Override
    public int getType() {
        return BottomSheetContentController.TYPE_INCOGNITO_HOME;
    }
}
