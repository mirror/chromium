// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.model;

import android.support.annotation.Nullable;

/**
 * XXX: Add Javadoc
 */
public interface ListObserver {
    /**
     * Notifies that {@code count} items starting at position {@code index} under the {@code child}
     * have been added.
     * @param source The list to which items have been added.
     * @param index The starting position of the range of added items, relative to the child.
     * @param count The number of added items.
     * @see android.support.v7.widget.RecyclerView.Adapter#notifyItemRangeInserted(int, int)
     */
    default void
        onItemRangeInserted(ListObservable source, int index, int count) {}

    /**
     * Notifies that {@code count} items starting at position {@code index} under the {@code child}
     * have been removed.
     * @param source The list from which items have been removed.
     * @param index The starting position of the range of removed items, relative to the child.
     * @param count The number of removed items.
     * @see android.support.v7.widget.RecyclerView.Adapter#notifyItemRangeRemoved(int, int)
     */
    default void
        onItemRangeRemoved(ListObservable source, int index, int count) {}

    /**
     * Notifies that {@code count} items starting at position {@code index} under the {@code child}
     * have changed with an optional payload object.
     * @param source The list whose items have changed.
     * @param index The starting position of the range of changed items, relative to the
     *         {@code child}.
     * @param count The number of changed items.
     * @param payload Optional parameter, use {@code null} to identify a "full" update.
     * @see android.support.v7.widget.RecyclerView.Adapter#notifyItemRangeChanged(int, int, Object)
     */
    default void
        onItemRangeChanged(ListObservable source, int index, int count, @Nullable Object payload) {}
}
