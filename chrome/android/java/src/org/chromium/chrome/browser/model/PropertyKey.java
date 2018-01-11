// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.model;

/**
 * Base class for type-safe property keys. Derive from this class and create constant instances of
 * the subclass as keys for {@link PropertyObservable} and {@link PropertyObserver}.
 */
public class PropertyKey {
    protected PropertyKey() {}
}
