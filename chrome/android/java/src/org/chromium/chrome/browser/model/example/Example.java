// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.model.example;

import org.chromium.chrome.browser.model.PropertyKey;
import org.chromium.chrome.browser.model.PropertyObservable;

/**
 * XXX: Add Javadoc
 */
public class Example extends PropertyObservable<Example.ExampleProperty> {
    static class ExampleProperty extends PropertyKey {}

    public static final ExampleProperty FOO = new ExampleProperty();
    public static final ExampleProperty BAR = new ExampleProperty();

    public void blurgleFoo() {
        notifyPropertyChanged(FOO);
    }
}
