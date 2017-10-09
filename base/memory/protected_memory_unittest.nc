// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a "No Compile Test" suite.
// http://dev.chromium.org/developers/testing/no-compile-tests

#include "base/memory/protected_memory.h"

namespace base {

#if defined(NCTEST_PROTECTEDMEMORYCALL_WITHOUT_PROTECTEDMEMORY)  // [r"ProtectedCall must use protected memory container"]

struct Foo {
  void (*fp)();
};

void WontCompile() {
  struct Foo *foo;
  ProtectedMemoryMemberCall(foo, fp);
}

#endif

} // namespace base
