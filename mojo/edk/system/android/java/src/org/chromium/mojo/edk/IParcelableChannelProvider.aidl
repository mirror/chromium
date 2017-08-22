// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.mojo.edk;
import org.chromium.mojo.edk.IParcelableChannel;
/** Used by a child process to send its IParcelableChannel to its parent. */
interface IParcelableChannelProvider {
    oneway void sendParcelableChannel(IParcelableChannel channel);
}
