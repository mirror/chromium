// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MAC_ASSERTION_HANDLER_H_
#define CHROME_BROWSER_MAC_ASSERTION_HANDLER_H_

namespace chrome {

// The NSAssert family of macros expand to an invocation of NSAssertionHandler,
// for which the current instance is stored in the thread-local
// -[NSThread threadDictionary]. The default implementation raises a
// NSException, which could be swallowed or re-thrown by various exception
// handlers.
//
// This function sets or replaces the current thread's NSAssertionHandler
// with one that calls LOG(FATAL) to terminate the process.
void InstallNSAssertionHandlerOnThread();

}  // namespace chrome

#endif  // CHROME_BROWSER_MAC_ASSERTION_HANDLER_H_
