// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import org.chromium.chrome.browser.IntentHandler;

/**
 * The MediaViewerActivity handles media-viewing Intents from other apps. It takes the given
 * content:// URI from the Intent and properly routes it to a media-viewing CustomTabActivity.
 */
public class MediaViewerActivity extends Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent input = getIntent();
        Uri contentUri = input.getData();
        String mimeType = getContentResolver().getType(contentUri);

        // TODO(steimel): Determine file:// URI when possible.
        Intent intent = MediaViewerUtils.getMediaViewerIntent(
                contentUri, contentUri, mimeType, false /* allowExternalAppHandlers */);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        IntentHandler.startActivityForTrustedIntent(intent);

        finish();
    }
}
