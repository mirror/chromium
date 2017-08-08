// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordUserAction;

import java.util.ArrayList;
import java.util.List;

/**
 * A util class that records UserActions.
 */
public class UserActionTester implements RecordUserAction.UserActionCallback {
    private List<String> mActions;

    public UserActionTester() {
        mActions = new ArrayList<>();
        ThreadUtils.runOnUiThreadBlocking(
                () -> RecordUserAction.setActionCallbackForTesting(UserActionTester.this));
    }

    public void tearDown() {
        ThreadUtils.runOnUiThreadBlocking(() -> RecordUserAction.removeActionCallbackForTesting());
    }

    @Override
    public void onActionRecorded(String action) {
        mActions.add(action);
    }

    public List<String> getActions() {
        return mActions;
    }

    @Override
    public String toString() {
        return "Actions: " + mActions.toString();
    }
}
