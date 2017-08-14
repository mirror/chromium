// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.support.annotation.DimenRes;
import android.support.annotation.IntDef;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.TitleUtil;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * The view for a site suggestion tile. Displays the title of the site beneath a large icon. If a
 * large icon isn't available, displays a rounded rectangle with a single letter in its place.
 */
public class TileView extends FrameLayout {
    @IntDef({Style.CLASSIC, Style.CONDENSED, Style.MODERN, Style.MODERN_CONDENSED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface Style {
        int CLASSIC = 0;
        int CONDENSED = 1;
        int MODERN = 2;
        int MODERN_CONDENSED = 3;
    }

    /** The url currently associated to this tile. */
    private SiteSuggestion mSiteData;

    private TextView mTitleView;
    private View mIconBackgroundView;
    private ImageView mIconView;
    private View mHighlightView;
    private ImageView mBadgeView;

    /**
     * Constructor for inflating from XML.
     */
    public TileView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mTitleView = findViewById(R.id.tile_view_title);
        mIconBackgroundView = findViewById(R.id.tile_view_icon_background);
        mIconView = findViewById(R.id.tile_view_icon);
        mHighlightView = findViewById(R.id.tile_view_highlight);
        mBadgeView = findViewById(R.id.offline_badge);
    }

    /**
     * Initializes the view using the data held by {@code tile}. This should be called immediately
     * after inflation.
     * @param tile The tile that holds the data to populate this view.
     * @param titleLines The number of text lines to use for the tile title.
     * @param tileStyle The visual style of the tile.
     */
    public void initialize(Tile tile, int titleLines, @Style int tileStyle) {
        mTitleView.setLines(titleLines);
        mSiteData = tile.getData();

        initRootView(tileStyle);
        initBackgroundView(tileStyle);
        initIconView(tileStyle);
        initHighlightView(tileStyle);
        initTitleView(tileStyle);

        mTitleView.setText(TitleUtil.getTitleForDisplay(tile.getTitle(), tile.getUrl()));
        renderOfflineBadge(tile);
        renderIcon(tile);
    }

    private void initRootView(@Style int tileStyle) {
        if (tileStyle == Style.CONDENSED || tileStyle == Style.MODERN_CONDENSED) {
            setPadding(0, 0, 0, 0);
            LayoutParams tileParams = (LayoutParams) getLayoutParams();
            tileParams.width = getPixels(R.dimen.tile_view_width_condensed);
            setLayoutParams(tileParams);
        }
    }

    private void initBackgroundView(@Style int tileStyle) {
        if (tileStyle == Style.MODERN || tileStyle == Style.MODERN_CONDENSED) {
            mIconBackgroundView.setVisibility(View.VISIBLE);
        }
    }

    private void initIconView(@Style int tileStyle) {
        if (tileStyle == Style.CLASSIC) return;

        LayoutParams iconParams = (LayoutParams) mIconView.getLayoutParams();

        if (tileStyle == Style.MODERN || tileStyle == Style.MODERN_CONDENSED) {
            iconParams.width = getPixels(R.dimen.tile_view_icon_size_modern);
            iconParams.height = getPixels(R.dimen.tile_view_icon_size_modern);
            iconParams.setMargins(0, getPixels(R.dimen.tile_view_icon_margin_top_modern), 0, 0);
        } else if (tileStyle == Style.CONDENSED) {
            iconParams.setMargins(0, getPixels(R.dimen.tile_view_icon_margin_top_condensed), 0, 0);
        }

        mIconView.setLayoutParams(iconParams);
    }

    private void initHighlightView(@Style int tileStyle) {
        if (tileStyle == Style.CONDENSED) {
            LayoutParams highlightParams = (LayoutParams) mHighlightView.getLayoutParams();
            highlightParams.setMargins(
                    0, getPixels(R.dimen.tile_view_icon_margin_top_condensed), 0, 0);
            mHighlightView.setLayoutParams(highlightParams);
        }
    }

    private void initTitleView(@Style int tileStyle) {
        if (tileStyle == Style.CONDENSED) {
            LayoutParams titleParams = (LayoutParams) mTitleView.getLayoutParams();
            titleParams.setMargins(
                    0, getPixels(R.dimen.tile_view_title_margin_top_condensed), 0, 0);
            mTitleView.setLayoutParams(titleParams);
        }
    }

    private int getPixels(@DimenRes int id) {
        return getResources().getDimensionPixelOffset(id);
    }

    /** @return The url associated with this view. */
    public String getUrl() {
        return mSiteData.url;
    }

    public SiteSuggestion getData() {
        return mSiteData;
    }

    /** @return The {@link TileSource} of the tile represented by this TileView */
    public int getTileSource() {
        return mSiteData.source;
    }

    /**
     * Renders the icon held by the {@link Tile} or clears it from the view if the icon is null.
     */
    public void renderIcon(Tile tile) {
        mIconView.setImageDrawable(tile.getIcon());
    }

    /** Shows or hides the offline badge to reflect the offline availability of the {@link Tile}. */
    public void renderOfflineBadge(Tile tile) {
        mBadgeView.setVisibility(tile.isOfflineAvailable() ? VISIBLE : GONE);
    }
}
