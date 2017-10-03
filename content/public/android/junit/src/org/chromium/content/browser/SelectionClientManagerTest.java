// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import static org.junit.Assert.assertFalse;
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

    private Runnable mNotifyBothClientsActiveRunnable;
    private boolean mDidRunNotifyBothSmartSelectAndContextualSearch;

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
        mManager = new SelectionClientManager();
        mSmartSelectionClientStub = new SmartSelectionClientStub();
        mDidRunNotifyBothSmartSelectAndContextualSearch = false;
        mNotifyBothClientsActiveRunnable = new Runnable() {
            @Override
            public void run() {
                mDidRunNotifyBothSmartSelectAndContextualSearch = true;
            }
        };
    }

    private boolean didRunBothActiveNotification() {
        return mDidRunNotifyBothSmartSelectAndContextualSearch;
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testNoSelectionClients() {
        assertFalse(mManager.requestSelectionPopupUpdates(false));
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testOneSelectionClient() {
        mManager.setSelectionClient(mSmartSelectionClientStub);

        assertTrue(mManager.requestSelectionPopupUpdates(false));
        assertTrue(mSmartSelectionClientStub.didCallRequestSelectionPopupUpdates());
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testTwoSelectionClients() {
        mManager.setSelectionClient(mSmartSelectionClientStub);
        SelectionClientStub contextualSearchSelectionClient = new SelectionClientStub();
        mManager.addSelectionClient(
                contextualSearchSelectionClient, mNotifyBothClientsActiveRunnable);
        assertTrue(didRunBothActiveNotification());

        assertTrue(mManager.requestSelectionPopupUpdates(false));
        assertTrue(mSmartSelectionClientStub.didCallRequestSelectionPopupUpdates());

        mManager.onSelectionChanged("unused");
        assertTrue(mSmartSelectionClientStub.didCallSelectionChanged());
        assertTrue(contextualSearchSelectionClient.didCallSelectionChanged());
    }

    @Test(expected = AssertionError.class)
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCantSetTwice() {
        mManager.setSelectionClient(mSmartSelectionClientStub);
        mManager.setSelectionClient(mSmartSelectionClientStub);
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCanAddWithoutSet() {
        mManager.addSelectionClient(new SelectionClientStub(), mNotifyBothClientsActiveRunnable);
        assertFalse(didRunBothActiveNotification());
    }

    @Test(expected = AssertionError.class)
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCantRemoveWithoutAdd() {
        mManager.removeSelectionClient(new SelectionClientStub());
    }

    @Test(expected = AssertionError.class)
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCantRemoveWrongClient() {
        mManager.addSelectionClient(new SelectionClientStub(), mNotifyBothClientsActiveRunnable);
        mManager.removeSelectionClient(new SelectionClientStub());
    }

    @Test
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCanRemoveIfAdded() {
        SelectionClient someClient = new SelectionClientStub();
        mManager.addSelectionClient(someClient, mNotifyBothClientsActiveRunnable);
        mManager.removeSelectionClient(someClient);
    }

    @Test(expected = AssertionError.class)
    @Feature({"TextInput", "SelectionClientManager"})
    public void testCantAddTwice() {
        SelectionClient someClient = new SelectionClientStub();
        mManager.addSelectionClient(someClient, mNotifyBothClientsActiveRunnable);
        mManager.addSelectionClient(someClient, mNotifyBothClientsActiveRunnable);
    }
}
