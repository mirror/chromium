// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import android.os.Handler;
import android.os.Looper;
import android.support.test.filters.SmallTest;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.View;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseActivityInstrumentationTestCase;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.chrome.browser.widget.selection.SelectableItemView;
import org.chromium.chrome.browser.widget.selection.SelectableItemViewHolder;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate.SelectionObserver;
import org.chromium.chrome.test.util.ChromeRestriction;

import java.util.Date;
import java.util.List;

/**
 * Tests the {@link HistoryActivity}.
 */
@Restriction(ChromeRestriction.RESTRICTION_TYPE_PHONE)
public class HistoryActivityTest extends BaseActivityInstrumentationTestCase<HistoryActivity> {
    private static class TestObserver extends RecyclerView.AdapterDataObserver
            implements SelectionObserver<HistoryItem> {
        public final CallbackHelper onChangedCallback = new CallbackHelper();
        public final CallbackHelper onSelectionCallback = new CallbackHelper();

        private Handler mHandler;

        public TestObserver() {
            mHandler = new Handler(Looper.getMainLooper());
        }

        @Override
        public void onChanged() {
            // To guarantee that all real Observers have had a chance to react to the event, post
            // the CallbackHelper.notifyCalled() call.
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    onChangedCallback.notifyCalled();
                }
            });
        }

        @Override
        public void onSelectionStateChange(List<HistoryItem> selectedItems) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    onSelectionCallback.notifyCalled();
                }
            });
        }
    }

    private StubbedHistoryProvider mHistoryProvider;
    private HistoryAdapter mAdapter;
    private HistoryManager mHistoryManager;
    private RecyclerView mRecyclerView;
    private TestObserver mTestObserver;

    private HistoryItem mItem1;
    private HistoryItem mItem2;

    public HistoryActivityTest() {
        super(HistoryActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        mHistoryProvider = new StubbedHistoryProvider();

        Date today = new Date();
        long[] timestamps = {today.getTime()};
        mItem1 = StubbedHistoryProvider.createHistoryItem(0, timestamps);
        mItem2 = StubbedHistoryProvider.createHistoryItem(1, timestamps);
        mHistoryProvider.addItem(mItem1);
        mHistoryProvider.addItem(mItem2);

        HistoryManager.setProviderForTests(mHistoryProvider);

        final HistoryActivity activity = getActivity();
        mHistoryManager = activity.getHistoryManagerForTests();
        mAdapter = mHistoryManager.getAdapterForTests();
        mTestObserver = new TestObserver();
        mHistoryManager.getSelectionDelegateForTests().addObserver(mTestObserver);
        mAdapter.registerAdapterDataObserver(mTestObserver);
        mRecyclerView = ((RecyclerView) activity.findViewById(R.id.recycler_view));

        assertEquals(4, mAdapter.getItemCount());
    }

    @SmallTest
    public void testRemove_SingleItem() throws Exception {
        int callCount = mTestObserver.onChangedCallback.getCallCount();
        final SelectableItemView<HistoryItem> itemView = getItemView(2);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                ((TintedImageButton) itemView.findViewById(R.id.remove)).performClick();
            }
        });

        // Check that one item was removed.
        mTestObserver.onChangedCallback.waitForCallback(callCount, 1);
        assertEquals(1, mHistoryProvider.markItemForRemovalCallback.getCallCount());
        assertEquals(1, mHistoryProvider.removeItemsCallback.getCallCount());
        assertEquals(3, mAdapter.getItemCount());
        assertEquals(View.VISIBLE, mRecyclerView.getVisibility());
        assertEquals(View.GONE, mHistoryManager.getEmptyViewForTests().getVisibility());
    }

    @SmallTest
    public void testRemove_AllItems() throws Exception {
        toggleItemSelection(2);
        toggleItemSelection(3);

        int callCount = mTestObserver.onChangedCallback.getCallCount();

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertTrue(mHistoryManager.getToolbarForTests().getMenu()
                        .performIdentifierAction(R.id.selection_mode_delete_menu_id, 0));
            }
        });

        // Check that all items were removed. The onChangedCallback should be called three times -
        // once for each item that is being removed and once for the removal of the header.
        mTestObserver.onChangedCallback.waitForCallback(callCount, 3);
        assertEquals(0, mAdapter.getItemCount());
        assertEquals(2, mHistoryProvider.markItemForRemovalCallback.getCallCount());
        assertEquals(1, mHistoryProvider.removeItemsCallback.getCallCount());
        assertFalse(mHistoryManager.getSelectionDelegateForTests().isSelectionEnabled());
        assertEquals(View.GONE, mRecyclerView.getVisibility());
        assertEquals(View.VISIBLE, mHistoryManager.getEmptyViewForTests().getVisibility());
    }

    private void toggleItemSelection(int position) throws Exception {
        int callCount = mTestObserver.onSelectionCallback.getCallCount();
        final SelectableItemView<HistoryItem> itemView = getItemView(position);
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                itemView.performLongClick();
            }
        });
        mTestObserver.onSelectionCallback.waitForCallback(callCount, 1);
    }

    @SuppressWarnings("unchecked")
    private SelectableItemView<HistoryItem> getItemView(int position) {
        ViewHolder mostRecentHolder = mRecyclerView.findViewHolderForAdapterPosition(position);
        assertTrue(mostRecentHolder instanceof SelectableItemViewHolder);
        return ((SelectableItemViewHolder<HistoryItem>) mostRecentHolder).getItemView();
    }
}
