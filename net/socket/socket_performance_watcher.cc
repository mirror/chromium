// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/socket_performance_watcher.h"

namespace net {

bool SocketPerformanceWatcher::AllowPrivateSockets() {
  return false;
}

}  // namespace net