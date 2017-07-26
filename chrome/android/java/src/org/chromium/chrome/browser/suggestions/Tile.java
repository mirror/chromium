// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.graphics.drawable.Drawable;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import org.chromium.chrome.browser.ntp.cards.ImpressionTracker;

/**
 * Holds the details to populate a site suggestion tile.
 */
public class Tile implements OfflinableSuggestion {
    private Data mData;

    @Nullable
    private Integer mIndex;

    @TileVisualType
    private int mType = TileVisualType.NONE;

    @Nullable
    private Drawable mIcon;

    @Nullable
    private Long mOfflinePageOfflineId;

    @Nullable
    private ImpressionTracker mImpressionTracker;

    public Tile(Data data) {
        mData = data;
    }

    @Override
    public String getUrl() {
        return mData.url;
    }

    @Override
    public void setOfflinePageOfflineId(@Nullable Long offlineId) {
        mOfflinePageOfflineId = offlineId;
    }

    @Nullable
    @Override
    public Long getOfflinePageOfflineId() {
        return mOfflinePageOfflineId;
    }

    @Override
    public boolean requiresExactOfflinePage() {
        return false;
    }

    /**
     * @return The title of this tile.
     */
    public String getTitle() {
        return mData.title;
    }

    /**
     * @return The path of the whitelist icon associated with the URL.
     */
    public String getWhitelistIconPath() {
        return mData.whitelistIconPath;
    }

    /**
     * @return Whether this tile is available offline.
     */
    public boolean isOfflineAvailable() {
        return getOfflinePageOfflineId() != null;
    }

    /** @return The position the tile is displayed at, or {@code null} if it's hidden. */
    public Integer getIndex() {
        return mIndex;
    }

    /** The position the tile is displayed at, or {@code null} if it's hidden. */
    public void setIndex(@Nullable Integer index) {
        mIndex = index;
    }

    /**
     * @return The source of this tile. Used for metrics tracking. Valid values are listed in
     * {@code TileSource}.
     */
    @TileSource
    public int getSource() {
        return mData.source;
    }

    /**
     * @return The visual type of this tile. Valid values are listed in {@link TileVisualType}.
     */
    @TileVisualType
    public int getType() {
        return mType;
    }

    /**
     * Sets the visual type of this tile. Valid values are listed in
     * {@link TileVisualType}.
     */
    public void setType(@TileVisualType int type) {
        mType = type;
    }

    /**
     * @return The icon, may be null.
     */
    @Nullable
    public Drawable getIcon() {
        return mIcon;
    }

    /**
     * Updates the icon drawable.
     */
    public void setIcon(@Nullable Drawable icon) {
        mIcon = icon;
    }

    public Data getData() {
        return mData;
    }

    public static final class Data {
        public final String title;
        public final String url;
        public final String whitelistIconPath;
        @TileSource
        public final int source;

        public Data(String title, String url, String whitelistIconPath, @TileSource int source) {
            this.title = title;
            this.url = url;
            this.whitelistIconPath = whitelistIconPath;
            this.source = source;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Data data = (Data) o;

            if (source != data.source) return false;
            if (!title.equals(data.title)) return false;
            if (!url.equals(data.url)) return false;
            return whitelistIconPath.equals(data.whitelistIconPath);
        }

        @Override
        public int hashCode() {
            int result = title.hashCode();
            result = 31 * result + source;
            result = 31 * result + url.hashCode();
            result = 31 * result + whitelistIconPath.hashCode();
            return result;
        }
    }
}
