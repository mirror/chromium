// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.learning.Features;

import java.net.URL;

/**
 * Implements the UMA logging for Ranker that's used for Contextual Search Tap Suppression.
 */
public class ContextualSearchRankerLoggerBrellaImpl implements ContextualSearchRankerLogger {

    private Features features;
    
    /**
     * Constructs a Ranker Logger and associated native implementation to write Contextual Search
     * ML data to Ranker.
     */
    public ContextualSearchRankerLoggerBrellaImpl() {
        // Set up GMS Core.
    }

    @Override
    public void setupLoggingForPage(URL basePageUrl) {
    }

    @Override
    public void logFeature(Feature feature, Object value) {
    }

    @Override
    public void logOutcome(Feature feature, Object value) {
    }

    @Override
    public boolean inferUiSuppression() {
        return false;
    }

    @Override
    public boolean wasUiSuppressionInfered() {
        return false;
    }

    @Override
    public void reset() {
    }

    @Override
    public void writeLogAndReset() {
        // Write
        
    }
}
