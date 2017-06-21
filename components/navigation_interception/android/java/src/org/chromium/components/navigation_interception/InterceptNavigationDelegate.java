// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.navigation_interception;

import org.chromium.base.annotations.CalledByNative;

/**
 * Interface used by embedder to control, or get notified of event from the ongoing navigation.
 */
public interface InterceptNavigationDelegate {

    /**
     * This method is called for every top-level navigation within the associated WebContents.
     * The method allows the embedder to ignore navigations. This is used on Android to 'convert'
     * certain navigations to Intents to 3rd party applications.
     *
     * @param navigationParams parameters describing the navigation.
     * @return true if the navigation should be ignored.
     */
    @CalledByNative
    boolean shouldIgnoreNavigation(NavigationParams navigationParams);

    /**
     * Notifies that a navigation operation is finished, by either committed or canceled.
     */
    @CalledByNative
    void navigationFinished();
}
