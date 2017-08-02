// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef LeakDetectorClient_h

#define LeakDetectorClient_h

namespace blink {

class LeakDetectorClient {
 public:
  virtual void OnLeakDetectionComplete() = 0;
};

}  // namespace blink

#endif  // LeakDetectorClient_h
