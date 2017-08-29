// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebURLRequestsTracker_h
#define WebURLRequestsTracker_h

namespace blink {

// This interface is for tracking resource request loading operations. Once
// all loading operations are done, the instance should be destroyed.
class WebURLRequestsTracker {
 public:
  virtual ~WebURLRequestsTracker() {}
};

}  // namespace blink

#endif  // WebURLRequestsTracker_h
