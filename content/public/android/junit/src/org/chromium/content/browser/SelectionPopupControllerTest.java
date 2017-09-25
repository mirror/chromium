// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isA;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.PackageManager;
import android.view.ActionMode;
import android.view.View;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;

import org.chromium.base.test.util.Feature;
import org.chromium.content_public.browser.WebContents;
import org.chromium.testing.local.LocalRobolectricTestRunner;
import org.chromium.ui.base.WindowAndroid;

/**
 * Unit tests for {@SelectionPopupController}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SelectionPopupControllerTest {
    SelectionPopupController mController;
    Context mContext;
    WindowAndroid mWindowAndroid;
    WebContents mWebContents;
    View mView;
    RenderCoordinates mRenderCoordinates;
    ActionMode mActionMode;
    PackageManager mPackageManager;

    class MySelectionClient implements SelectionClient {
        private SelectionClient.Result mResult;
        private SelectionClient.ResultCallback mResultCallback;

        @Override
        public void onSelectionChanged(String selection) {}

        @Override
        public void onSelectionEvent(int eventType, float posXPix, float poxYPix) {}

        @Override
        public void showUnhandledTapUIIfNeeded(int x, int y) {}

        @Override
        public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {}

        @Override
        public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
            mResultCallback.onClassified(mResult);
            return true;
        }

        @Override
        public void cancelAllRequests() {}

        public void setResult(SelectionClient.Result result) {
            mResult = result;
        }

        public void setResultCallback(SelectionClient.ResultCallback callback) {
            mResultCallback = callback;
        }
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        ShadowLog.stream = System.out;

        mContext = Mockito.mock(Context.class);
        mWindowAndroid = Mockito.mock(WindowAndroid.class);
        mWebContents = Mockito.mock(WebContents.class);
        mView = Mockito.mock(View.class);
        mRenderCoordinates = Mockito.mock(RenderCoordinates.class);
        mActionMode = Mockito.mock(ActionMode.class);
        mPackageManager = Mockito.mock(PackageManager.class);

        when(mContext.getPackageManager()).thenReturn(mPackageManager);

        mController = SelectionPopupController.createForTesting(
                mContext, mWindowAndroid, mWebContents, mView, mRenderCoordinates);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSmartSelectionAdjustSelectionRange() {
        InOrder order = inOrder(mWebContents, mView);

        // Long press "Amphitheatre" in "1600 Amphitheatre Parkway".
        SelectionClient.Result result = new SelectionClient.Result();
        result.startAdjust = -5;
        result.endAdjust = 8;
        result.label = "Maps";

        // Setup SelectionClient for SelectionPopupController.
        MySelectionClient client = new MySelectionClient();
        client.setResult(result);
        client.setResultCallback(mController.getResultCallback());
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, "Amphitheatre", /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                /* fromSelectionAdjustment = */ false);

        // adjustSelectionByCharacterOffset() should be called.
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(result.startAdjust, result.endAdjust, true);
        assertFalse(mController.isActionModeValid());

        when(mView.startActionMode(any(FloatingActionModeCallback.class), anyInt()))
                .thenReturn(mActionMode);

        // Call showSelectionMenu again, which is adjustSelectionByCharacterOffset triggered.
        mController.showSelectionMenu(0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, "1600 Amphitheatre Parkway",
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                /* fromSelectionAdjustment = */ true);

        order.verify(mView).startActionMode(
                isA(FloatingActionModeCallback.class), eq(ActionMode.TYPE_FLOATING));
        assertTrue(mController.isActionModeValid());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSmartSelectionMultiRangeAdjustment() {
        InOrder order = inOrder(mWebContents, mView);

        // Longpress "Amphitheatre" in "1600 Amphitheatre Parkway".
        SelectionClient.Result result = new SelectionClient.Result();
        result.startAdjust = -5;
        result.endAdjust = 8;
        result.label = "Maps";

        // Set SelectionClient for SelectionPopupController.
        MySelectionClient client = new MySelectionClient();
        client.setResult(result);
        client.setResultCallback(mController.getResultCallback());
        mController.setSelectionClient(client);

        // Long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, "Amphitheatre", /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                /* fromSelectionAdjustment = */ false);

        // adjustSelectionByCharacterOffset() should be called.
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(result.startAdjust, result.endAdjust, true);
        assertFalse(mController.isActionModeValid());

        // Long press "Mountain" in "585 Franklin Street, Mountain View, CA 94041".
        SelectionClient.Result newResult = new SelectionClient.Result();
        newResult.startAdjust = -21;
        newResult.endAdjust = 15;
        newResult.label = "Maps";

        client.setResult(newResult);

        // Another long press triggered showSelectionMenu() call.
        mController.showSelectionMenu(0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, "Mountain", /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                /* fromSelectionAdjustment = */ false);
        order.verify(mWebContents)
                .adjustSelectionByCharacterOffset(newResult.startAdjust, newResult.endAdjust, true);
        assertFalse(mController.isActionModeValid());

        when(mView.startActionMode(any(FloatingActionModeCallback.class), anyInt()))
                .thenReturn(mActionMode);

        // First adjustSelectionByCharacterOffset() triggered.
        mController.showSelectionMenu(0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, "1600 Amphitheatre Parkway",
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                /* fromSelectionAdjustment = */ true);

        SelectionClient.Result returnResult = mController.getClassificationResult();
        assertTrue(returnResult.startAdjust == newResult.startAdjust
                && returnResult.endAdjust == newResult.endAdjust
                && returnResult.label == newResult.label);

        // Second adjustSelectionByCharacterOffset() triggered.
        mController.showSelectionMenu(0, 0, 0, 0, /* isEditable = */ true,
                /* isPasswordType = */ false, "585 Franklin Street, Mountain View, CA 94041",
                /* canSelectAll = */ true,
                /* canRichlyEdit = */ true, /* shouldSuggest = */ true,
                /* fromSelectionAdjustment = */ true);

        order.verify(mView).startActionMode(
                isA(FloatingActionModeCallback.class), eq(ActionMode.TYPE_FLOATING));
        assertTrue(mController.isActionModeValid());
    }
}
