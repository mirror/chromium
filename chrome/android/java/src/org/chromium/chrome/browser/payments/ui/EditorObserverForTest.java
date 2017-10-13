// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments.ui;

/**
 * A test-only observer for Editor UI.
 */
public interface EditorObserverForTest {
    /**
     * Called when edit dialog is showing.
     */
    void onEditorReadyToEdit();

    /**
     * Called when editor validation completes with error. This can happen, for example, when
     * user enters an invalid email address.
     */
    void onEditorValidationError();

    /**
     * Called when an editor field text has changed.
     */
    void onEditorTextUpdate();

    /**
     * Called when the UI is gone.
     */
    void onEditorDismiss();
}
