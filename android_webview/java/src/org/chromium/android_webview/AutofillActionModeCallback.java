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
    private final int mAutofillMenuItemId;
    private final int mAutofillMenuItemTitle;

    // TODO: Using show/hide to add/remove autofill menu once we switch to Android O MR1
    // See SelectionPopupController.MENU_ITEM_ORDER_TEXT_PROCESS_START.
    private static final int MENU_ITEM_ORDER_AUTOFILL_START = 200;

    public AutofillActionModeCallback(Context context, AwContents awContents) {
        mContext = context;
        mAwContents = awContents;
        mAutofillMenuItemId = mContext.getResources().getIdentifier("autofill", "id", "android");
        mAutofillMenuItemTitle = mContext.getResources().getIdentifier("autofill", "string", "android");
    }

    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
        return mAutofillMenuItemId != 0 && mAutofillMenuItemTitle !=0;
    }

    @Override
    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
        if (mAutofillMenuItemId != 0 && mAutofillMenuItemTitle != 0 && mAwContents.hasAutofillSession()) {
            menu.add(R.id.select_action_menu_autofill_menus, mAutofillMenuItemId, MENU_ITEM_ORDER_AUTOFILL_START, mAutofillMenuItemTitle);
        }
        return true;
    }

    @Override
    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
        if (item.getItemId() == mAutofillMenuItemId) {
            mAwContents.queryAutofillSuggestion();
            mode.finish();
            return true;
        }
        return false;
    }

    @Override
    public void onDestroyActionMode(ActionMode mode) {
    }

}
