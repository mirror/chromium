// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import org.chromium.base.MainReturnCodeResult;
import org.chromium.base.process_launcher.FileDescriptorInfo;

/**
 * This interface is used to control child processes.
 */
interface ITestController {
   /**
   * Blocks until the <code>main</code> method started with {@link #launch()} returns, or returns
   * immediately if main has already returned.
   * Note that you must call mainResultReceived() to unblock the service after calling this method.
   * @param timeoutMs time in milliseconds after which this method returns even if the main method
   * has not returned yet.
   * @return a result containing whether a timeout occured and the value returned by the
   * <code>main</code> method
   */
  MainReturnCodeResult waitForMainToReturn(long timeoutMs);

  /**
   * Must be called when using waitForMainToReturn and the MainReturnCodeResult has been received.
   * This is needed as the service will terminate as soon as main returns and the binder might not
   * have sent back the MainReturnCodeResult yet. So we need an acknoledgment from the client that
   * the result was indeed received so main can return and the service die.
   */
  oneway void mainResultReceived();

  /**
   * Forces the service process to terminate and block until the process stops.
   * @param exitCode the exit code the process should terminate with.
   * @return always true, a return value is only returned to force the call to be synchronous.
   */
  boolean forceStopSynchronous(int exitCode);

  /**
   * Forces the service process to terminate.
   * @param exitCode the exit code the process should terminate with.
   */
  oneway void forceStop(int exitCode);
}
