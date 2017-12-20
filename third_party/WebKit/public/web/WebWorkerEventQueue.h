// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebWorkerEventQueue_h
#define WebWorkerEventQueue_h

namespace blink {

class WebWorkerEventQueue {
 public:
  BLINK_EXPORT virtual void HandleInputEvent() = 0;
};

}  // namespace blink

#endif  // WebWorkerEventQueue_h
