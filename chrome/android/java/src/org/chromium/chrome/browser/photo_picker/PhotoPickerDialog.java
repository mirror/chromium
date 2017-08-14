// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.photo_picker;

import android.content.Context;
import android.support.v7.app.AlertDialog;
import android.view.View;

import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.ui.PhotoPickerListener;

import java.util.List;

/**
 * UI for the photo chooser that shows on the Android platform as a result of
 * &lt;input type=file accept=image &gt; form element.
 */
public class PhotoPickerDialog extends AlertDialog {
    // The category we're showing photos for.
    private PickerCategoryView mCategoryView;

    // TODOf doc.
    private boolean mDismissImmediately;

    /**
     * The PhotoPickerDialog constructor.
     * @param context The context to use.
     * @param listener The listener object that gets notified when an action is taken.
     * @param multiSelectionAllowed Whether the photo picker should allow multiple items to be
     *                              selected.
     * @param mimeTypes A list of mime types to show in the dialog.
     */
    public PhotoPickerDialog(Context context, PhotoPickerListener listener,
            boolean multiSelectionAllowed, List<String> mimeTypes) {
        super(context, R.style.FullscreenWhite);

        // Initialize the main content view.
        mCategoryView = new PickerCategoryView(context);
        mCategoryView.initialize(this, listener, multiSelectionAllowed, mimeTypes);
        setView(mCategoryView);
    }

    private void dismissLater() {
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
        }

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mDismissImmediately = true;
                dismiss();
            }
        });
    }

    @Override
    public void dismiss() {
        if (!mCategoryView.intentLaunched() || mDismissImmediately) {
            super.dismiss();
            mCategoryView.onDialogDismissed();
        } else {
            // Already in the cancel process, this prevents the Back button from starting another.
            mCategoryView.disableCancel();

            // Surface the black background.
            findViewById(R.id.selectable_list).setVisibility(View.GONE);
            findViewById(R.id.lights_out).setVisibility(View.VISIBLE);

            new Thread(new Runnable() {
                @Override
                public void run() {
                    dismissLater();
                }
            })
                    .start();
        }
    }

    @VisibleForTesting
    public PickerCategoryView getCategoryViewForTesting() {
        return mCategoryView;
    }
}
