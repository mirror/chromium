// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PlatformAXTree_h
#define PlatformAXTree_h

namespace blink {

class PlatformAXTree {
 public:
  ~PlatformAXTree() = default;

  virtual void RequestTreeSnapshot() = 0;
  virtual void Walk() = 0;
};

};  // namespace blink

#endif  // PlatformAXTree_h
