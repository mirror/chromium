// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.widget.bottomsheet;

import android.support.v7.widget.GridLayout;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.suggestions.SiteSectionViewHolder;
import org.chromium.chrome.browser.suggestions.SiteSuggestion;
import org.chromium.chrome.browser.suggestions.TileView;

import javax.annotation.Nullable;

/**
 * Created by galinap on 9/1/17.
 */

class LauncherSectionViewHolder extends SiteSectionViewHolder {
    private final GridLayout mGridLayout;

    public LauncherSectionViewHolder(ViewGroup parent) {
        super(LayoutInflater.from(parent.getContext())
                        .inflate(org.chromium.chrome.R.layout.launcher_grid_layout, parent, false));
        mGridLayout = itemView.findViewById(R.id.grid_layout);
    }

    @Override
    public void refreshData() {}

    @Nullable
    @Override
    protected TileView findTileView(SiteSuggestion data) {
        GridLayout gridLayout = itemView.findViewById(R.id.grid_layout);

        TileView tileView = null;
        for (int i = 0; i < gridLayout.getChildCount(); i++) {
            tileView = (TileView) gridLayout.getChildAt(i);

            if (tileView.getData() == data) {
                break;
            }
        }

        return tileView;
    }
}
