// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.support.annotation.DrawableRes;

/**
 * Defines the appearance and callbacks for {@link NotifyInfoBar}.
 */
interface NotifyInfoBarDelegate {
    /** The full description of the notification, visible when the infobar is expanded. */
    String getDescription();

    /** The short description of the notification, visible when the infobar is collapsed. */
    String getShortDescription();

    /** The text for the link displayed when the infobar is expanded. */
    String getFeaturedLinkText();

    /** The icon to show in this infobar. */
    @DrawableRes
    int getIconResourceId();

    /** Callback called when the featured link is tapped. */
    void onLinkTapped();
}
