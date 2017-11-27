// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.selection.SelectableListToolbar;

/**
 * A simply customized SelectionToolbar for the language selection UI.
 * TODO(crbug/783049): We only enabled "Close" and "Search" for now, extending this
 * class for its other features later.
 */
public class LanguageSelectionToolbar extends SelectableListToolbar<LanguageItem> {
    public LanguageSelectionToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);
        inflateMenu(R.menu.languages_action_bar_menu);
    }
}
