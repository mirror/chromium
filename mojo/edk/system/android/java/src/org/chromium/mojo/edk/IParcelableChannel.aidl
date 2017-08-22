// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.mojo.edk;
import org.chromium.mojo.edk.ParcelableWrapper;
/**
 * This interface is used to send Java parcelable objects to child processes.
 */
interface IParcelableChannel {
  oneway void sendParcelable(in ParcelableWrapper parcelableValue);
}
