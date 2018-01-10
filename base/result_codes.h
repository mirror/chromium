// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_RESULT_CODES_H_
#define BASE_RESULT_CODES_H_

namespace base {

// The return code is the value that:
// a) is returned by main() or winmain(), or
// b) specified in the call for ExitProcess() or TerminateProcess(), or
// c) the exception value that causes a process to terminate.
//
// It is advisable to not use negative numbers because the Windows API returns
// it as an unsigned long and the exception values have high numbers. For
// example EXCEPTION_ACCESS_VIOLATION value is 0xC0000005.
enum ResultCode {
  // Process terminated normally.
  RESULT_CODE_NORMAL_EXIT,

  // Process was killed by user or system.
  RESULT_CODE_KILLED,

  // Process hung.
  RESULT_CODE_HUNG,

  // Last return code (keep this last).
  RESULT_CODE_LAST_CODE
};

}  // namespace base

#endif  // BASE_RESULT_CODES_H_
