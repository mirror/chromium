// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.content.browser.ContentClassFactory;

/**
 * A public API for ContentClassFactory.
 */
public class ContentClassFactoryInstaller {
    public static void installFactory() {
        ContentClassFactory.set(new ContentClassFactory());
    }
}
