// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_EXTRAS_STATUS_LISTENER_H_
#define NET_EXTRAS_STATUS_LISTENER_H_

#include "base/callback_forward.h"

namespace net {

// StatusListener provides a simple generic interface to react when the
// application (as defined by an implementation of StatusListener) changes
// state. Callbacks that are provided for the StatusListener to run must live
// for the lifetime of the StatusListener, but will not be run after the
// StatusListener has been destroyed.
//
// Implementations of StatusListener are not thread-safe: the constructor,
// destructor, and SetApplicationStoppedCallback must all be called on the same
// thread.
class StatusListener {
 public:
  virtual ~StatusListener() {}

  // SetApplicationStoppedCallback sets |callback| to be run each time the
  // application transitions into a stopped state (as defined by the specific
  // StatusListener).
  virtual void SetApplicationStoppedCallback(
      base::RepeatingClosure callback) = 0;
};

}  // namespace net

#endif  // NET_EXTRAS_STATUS_LISTENER_H_
