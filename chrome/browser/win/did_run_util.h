// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WIN_DID_RUN_UTIL_H_
#define CHROME_BROWSER_WIN_DID_RUN_UTIL_H_

namespace base {
class CommandLine;
}

bool ShouldRecordActiveUse(const base::CommandLine& command_line);

#endif  // CHROME_BROWSER_WIN_DID_RUN_UTIL_H_
