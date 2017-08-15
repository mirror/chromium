// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.view.View;

import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;

/**
 * Describes a portion of UI responsible for rendering a group of sites. It abstracts general tasks
 * related to initialising and updating this UI.
 */
public abstract class SiteSectionViewHolder extends NewTabPageViewHolder {
    /**
     * Constructs a {@link NewTabPageViewHolder} used to display an part of the NTP (e.g., header,
     * article snippet, above-the-fold view, etc.)
     *
     * @param itemView The {@link View} for this item
     */
    public SiteSectionViewHolder(View itemView) {
        super(itemView);
    }

    /** Initialise the view, letting it know the data it will have to display. */
    public abstract void bindDataSource(TileGroup tileGroup, TileRenderer tileRenderer);

    /** Clears the current data and displays the current state of the model. */
    public abstract void refreshData();

    /** Updates the icon shown for the provided tile. */
    public abstract void updateIconView(Tile tile);

    /** Updates the offline badge for the provided tile. */
    public abstract void updateOfflineBadge(Tile tile);
}
