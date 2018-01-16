// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.app;

import org.chromium.base.BaseChromiumApplication;
import org.chromium.base.annotations.MainDex;

/**
 * Basic application functionality that should be shared among all browser applications
 * based on the content layer.
 */
@MainDex
public abstract class ContentApplication extends BaseChromiumApplication {

}
