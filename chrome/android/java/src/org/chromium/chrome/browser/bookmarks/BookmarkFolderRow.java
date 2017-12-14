// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;
import android.content.res.ColorStateList;
import android.util.AttributeSet;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.components.bookmarks.BookmarkId;

import java.util.List;

/**
 * A row view that shows folder info in the bookmarks UI.
 */
public class BookmarkFolderRow extends BookmarkRow {

    /**
     * Constructor for inflating from XML.
     */
    public BookmarkFolderRow(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        setIconDrawable(BookmarkUtils.getFolderIcon(getResources()));
    }

    // BookmarkRow implementation.

    @Override
    public void onClick() {
        mDelegate.openFolder(mBookmarkId);
    }

    @Override
    BookmarkItem setBookmarkId(BookmarkId bookmarkId) {
        BookmarkItem item = super.setBookmarkId(bookmarkId);
        mTitleView.setText(item.getTitle());
        int childCount = countBookmarks(bookmarkId);
        mDescriptionView.setText((childCount > 0)
                        ? getResources().getQuantityString(
                                  R.plurals.bookmarks_count, childCount, childCount)
                        : getResources().getString(R.string.no_bookmarks));
        return item;
    }

    @Override
    protected ColorStateList getDefaultIconTint() {
        return ApiCompatibilityUtils.getColorStateList(
                getResources(), BookmarkUtils.getFolderIconTint());
    }

    /**
     * @param bookmarkId The {@link BookmarkId} of the specified folder.
     * @return The total number of bookmarks inside the specified folder and its sub folders.
     */
    private int countBookmarks(BookmarkId bookmarkId) {
        BookmarkModel model = mDelegate.getModel();
        List<BookmarkId> subFolders = model.getChildIDs(bookmarkId, true, false);
        int count = model.getChildCount(bookmarkId) - subFolders.size();
        for (BookmarkId folder : subFolders) count += countBookmarks(folder);
        return count;
    }
}
