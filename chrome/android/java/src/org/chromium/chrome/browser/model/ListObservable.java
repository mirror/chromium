// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.model;

import android.support.annotation.Nullable;

import org.chromium.base.ObserverList;

/**
 * XXX: Add Javadoc
 */
public class ListObservable {
    private final ObserverList<ListObserver> mObservers = new ObserverList<>();

    public void addObserver(ListObserver observer) {
        mObservers.addObserver(observer);
    }

    public void removeObserver(ListObserver observer) {
        mObservers.removeObserver(observer);
    }

    protected void notifyItemRangeInserted(int index, int count) {
        for (ListObserver observer : mObservers) {
            observer.onItemRangeInserted(this, index, count);
        }
    }

    protected void notifyItemInserted(int index) {
        notifyItemRangeInserted(index, 1);
    }

    protected void notifyItemRangeRemoved(int index, int count) {
        for (ListObserver observer : mObservers) {
            observer.onItemRangeRemoved(this, index, count);
        }
    }

    protected void notifyItemRemoved(int index) {
        notifyItemRangeRemoved(index, 1);
    }

    protected void notifyItemRangeChanged(int index, int count, @Nullable Object payload) {
        for (ListObserver observer : mObservers) {
            observer.onItemRangeChanged(this, index, count, payload);
        }
    }

    protected void notifyItemRangeUpdated(int index, int count) {
        notifyItemRangeChanged(index, count, null);
    }
}
