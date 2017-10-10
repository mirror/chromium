// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.util.Feature;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for {@SelectionClientManager}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SelectionClientManagerTest {
    private SelectionClientManager mManager;

    private SmartSelectionClientStub mSmartSelectionClientStub;

    private class SelectionClientStub implements SelectionClient {
        boolean mDidCallSelectionChanged;

        boolean didCallSelectionChanged() {
            return mDidCallSelectionChanged;
        }

        @Override
        public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
            return true;
        }

        @Override
        public void onSelectionChanged(String selection) {
            mDidCallSelectionChanged = true;
        }

        @Override
        public void onSelectionEvent(int eventType, float posXPix, float posYPix) {}

        @Override
        public void showUnhandledTapUIIfNeeded(int x, int y) {}

        @Override
        public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {}

        @Override
        public void cancelAllRequests() {}
    }

    private class SmartSelectionClientStub extends SelectionClientStub {
        boolean mDidCallRequestSelectionPopupUpdates;

        boolean didCallRequestSelectionPopupUpdates() {
            return mDidCallRequestSelectionPopupUpdates;
        }

        @Override
        public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
            mDidCallRequestSelectionPopupUpdates = true;
            return true;
        }
    }

    @Before
    public void setUp() throws Exception {
        mSmartSelectionClientStub = new SmartSelectionClientStub();
        mManager = new SelectionClientManager(mSmartSelectionClientStub, true);
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testNoSelectionClients() {
        SelectionClientManager managerWithoutSmartSelection =
                new SelectionClientManager(null, true);
        assertNull(managerWithoutSmartSelection.getSelectionClient());
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testDisabledSelectionClientFlag() {
        SelectionClientManager managerWithDisabledSmartSelection =
                new SelectionClientManager(mSmartSelectionClientStub, false);
        assertFalse(managerWithDisabledSmartSelection.isSmartSelectionEnabledInChrome());
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testOneSelectionClient() {
        assertTrue(mManager.getSelectionClient().requestSelectionPopupUpdates(false));
        assertTrue(mSmartSelectionClientStub.didCallRequestSelectionPopupUpdates());
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testTwoSelectionClients() {
        SelectionClientStub contextualSearchSelectionClient = new SelectionClientStub();
        mManager.addContextualSearchSelectionClient(contextualSearchSelectionClient);

        assertTrue(mManager.getSelectionClient().requestSelectionPopupUpdates(false));
        assertTrue(mSmartSelectionClientStub.didCallRequestSelectionPopupUpdates());

        mManager.getSelectionClient().onSelectionChanged("unused");
        assertTrue(mSmartSelectionClientStub.didCallSelectionChanged());
        assertTrue(contextualSearchSelectionClient.didCallSelectionChanged());
    }

    @Test(expected = AssertionError.class)
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCantRemoveWithoutAdd() {
        mManager.removeContextualSearchSelectionClient();
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCanRemoveIfAdded() {
        SelectionClient someClient = new SelectionClientStub();
        mManager.addContextualSearchSelectionClient(someClient);
        mManager.removeContextualSearchSelectionClient();
    }

    @Test(expected = AssertionError.class)
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCantAddTwice() {
        SelectionClient someClient = new SelectionClientStub();
        mManager.addContextualSearchSelectionClient(someClient);
        mManager.addContextualSearchSelectionClient(someClient);
    }
}
