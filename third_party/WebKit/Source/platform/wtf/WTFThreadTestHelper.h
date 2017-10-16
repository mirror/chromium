// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WTFThreadTestHelper_h
#define WTFThreadTestHelper_h

#include "platform/wtf/Threading.h"
#include "platform/wtf/WTF.h"

namespace WTF {

// While this class is on the stack the value of IsMainThread will be inverted.
// This is for use in unittesting only.  For example where the thread
// sctructure is not set up, but our classes may be DCHECKing it for product
// stability.
class NotOnMainThread {
 public:
  NotOnMainThread();
  ~NotOnMainThread();

 private:
  ThreadIdentifier old_main_;
};

}  // namespace WTF

#endif  // WTF_WTFThreadTestHelper_h
