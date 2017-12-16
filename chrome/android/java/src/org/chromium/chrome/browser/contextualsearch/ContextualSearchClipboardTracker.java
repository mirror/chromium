// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.text.TextUtils;

import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.omnibox.UrlBar;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.feature_engagement.EventConstants;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.content.browser.SelectionPopupController;

/**
 * Helper class for keeping track of copy/paste from {@link
 * org.chromium.content_public.browser.WebContents} to omnibox.
 */

public class ContextualSearchClipboardTracker
        implements UrlBar.ActionItemObserver, SelectionPopupController.ActionItemObserver {
    /**
     * The maximum time allowed between copying and pasting in milliseconds, in order to notify the
     * event to the Feature Engagement backend.
     */
    private static final long CLIPBOARD_TIME_LIMIT = 15 * 60 * 1000;

    private String mCopiedText;
    private long mCopiedTime;

    /**
     * Called when the selected text on a web page is copied to clipboard.
     * @param selectedText The selected text.
     */
    @Override
    public void onOptionCopyClicked(String selectedText) {
        mCopiedText = selectedText;
        mCopiedTime = System.currentTimeMillis();
    }

    /**
     * Called when clipboard's content is pasted into the omnibox.
     * @param rawText Clipboard's content without formatting.
     * @param sanitizedText Sanitized content using {@link
     * OmniboxViewUtil#sanitizeTextForPaste(String)}.
     */
    @Override
    public void onOptionPasteClicked(String rawText, String sanitizedText) {
        if (TextUtils.isEmpty(mCopiedText) || TextUtils.isEmpty(rawText)) return;

        long now = System.currentTimeMillis();
        if (mCopiedText.equals(rawText) && now - mCopiedTime < CLIPBOARD_TIME_LIMIT) {
            Tracker tracker = TrackerFactory.getTrackerForProfile(
                    Profile.getLastUsedProfile().getOriginalProfile());
            tracker.notifyEvent(EventConstants.COPY_PASTE_SEARCH_PERFORMED);
        }
    }
}
