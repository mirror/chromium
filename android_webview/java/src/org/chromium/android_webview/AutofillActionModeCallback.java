// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.Context;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import org.chromium.content.R;

public class AutofillActionModeCallback implements ActionMode.Callback {
    private final Context mContext;
    private final AwContents mAwContents;
    private final int mAutofillMenuItemTitle;

    public AutofillActionModeCallback(Context context, AwContents awContents) {
        mContext = context;
        mAwContents = awContents;
        // Add title to content/public/android/java/res/menu/select_action_menu.xml after SDK rolls
        // to Android O MR1.
        mAutofillMenuItemTitle =
                mContext.getResources().getIdentifier("autofill", "string", "android");
    }

    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
        if (mAutofillMenuItemTitle == 0) return false;
        MenuItem autofillItem = menu.findItem(android.R.id.autofill);
        if (autofillItem != null) {
            autofillItem.setTitle(mContext.getResources().getString(mAutofillMenuItemTitle));
        }
        return true;
    }

    @Override
    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
        MenuItem autofillItem = menu.findItem(android.R.id.autofill);
        if (autofillItem != null && mAutofillMenuItemTitle != 0) {
            autofillItem.setVisible(mAwContents.hasAutofillSession());
        }
        return true;
    }

    @Override
    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
        if (item.getItemId() == android.R.id.autofill) {
            mAwContents.queryAutofillSuggestion();
            mode.finish();
            return true;
        }
        return false;
    }

    @Override
    public void onDestroyActionMode(ActionMode mode) {}
}
