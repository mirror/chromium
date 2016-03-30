// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell_apk;

import android.os.Bundle;

import com.google.vrtoolkit.cardboard.CardboardActivity;
import com.google.vrtoolkit.cardboard.CardboardView;

/**
 * Activity for managing the Content Shell.
 */
public class ContentCardboardActivity extends CardboardActivity {

    private ContentCardboardRenderer mRenderer;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.content_vr_shell_activity);
        CardboardView cardboardView = (CardboardView) findViewById(R.id.cardboard_view);
        mRenderer = new ContentCardboardRenderer(this);
        cardboardView.setRenderer(mRenderer);
        setCardboardView(cardboardView);
    }

    @Override
    public void onCardboardTrigger() {
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

}
