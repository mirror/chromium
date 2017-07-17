// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/socket_watcher_test_util.h"

namespace net {

namespace nqe {

namespace internal {

TestSocketWatcher::TestSocketWatcher(const SocketWatcher& socket_watcher)
    : SocketWatcher(socket_watcher) {}

TestSocketWatcher::~TestSocketWatcher() {}

bool TestSocketWatcher::AllowPrivateSockets() const {
  return true;
}

}  // namespace internal

}  // namespace nqe

}  // namespace net
