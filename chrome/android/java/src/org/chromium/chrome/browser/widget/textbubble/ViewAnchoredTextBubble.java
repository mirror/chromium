// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.textbubble;

import android.content.Context;
import android.support.annotation.StringRes;
import android.view.View;

/**
 * A helper class that anchors a {@link TextBubble} to a particular {@link View}.  The bubble will
 * listen to layout events on the {@link View} and update accordingly.
 */
public class ViewAnchoredTextBubble extends TextBubble {
    /**
     * Creates an instance of a {@link ViewAnchoredTextBubble}.
     * @param context    Context to draw resources from.
     * @param anchorView The {@link View} to anchor to.
     * @param stringId The id of the string resource for the text that should be shown.
     * @param accessibilityStringId The id of the string resource of the accessibility text.
     */
    // TODO -- REMOVE THIS CLASS
    public ViewAnchoredTextBubble(Context context, View anchorView, @StringRes int stringId,
            @StringRes int accessibilityStringId) {
        super(context, anchorView.getRootView(), stringId, accessibilityStringId);
    }
}