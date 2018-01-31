// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.test.component;

/**
 * Interface that the component must implement to be loaded by Chrome.
 */
public interface TestComponentInterface {
    /**
     * @since 1.0
     */
    int sum(int a, int b);
    /**
     * @since 2.0
     */
    String hello();
}