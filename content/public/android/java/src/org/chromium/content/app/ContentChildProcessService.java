// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.app;

import org.chromium.base.process_launcher.ChildProcessService;
import org.chromium.base.process_launcher.ChildProcessServiceDelegate;

/** Implementation of ChildProcessService that uses the content specific delegate. */
public class ContentChildProcessService extends ChildProcessService {
    protected ChildProcessServiceDelegate createDelegate() {
        return new ContentChildProcessServiceDelegate();
    }
}
