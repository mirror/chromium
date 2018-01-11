// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.model;

import android.support.annotation.Nullable;

/**
 * XXX: Add Javadoc
 * @param <T> XXX
 */
public interface PropertyObserver<T extends PropertyKey> {
    void onPropertyChanged(PropertyObservable<T> source, @Nullable T propertyKey);
}
