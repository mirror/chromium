// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WTF_WTFThreadTestHelper_h
#define WTF_WTFThreadTestHelper_h

#include "platform/wtf/WTF.h"

namespace WTF {

class NotOnMainThread {
 public:
  NotOnMainThread();
  ~NotOnMainThread();

 private:
  ThreadIdentifier old_;
};

}  // namespace WTF

#endif  // WTF_WTFThreadTestHelper_h
