// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.model;

import android.support.annotation.Nullable;

import org.chromium.base.ObserverList;

/**
 * XXX: Add Javadoc
 * @param <T> XXX
 */
public class PropertyObservable<T extends PropertyKey> {

    private final ObserverList<PropertyObserver<T>> mObservers = new ObserverList<>();

    public void addObserver(PropertyObserver<T> observer) {
        mObservers.addObserver(observer);
    }

    public void removeObserver(PropertyObserver<T> observer) {
        mObservers.removeObserver(observer);
    }

    protected void notifyPropertyChanged(@Nullable T propertyKey) {
        for (PropertyObserver<T> observer : mObservers) {
            observer.onPropertyChanged(this, propertyKey);
        }
    }

}
