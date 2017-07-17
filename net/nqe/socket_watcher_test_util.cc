// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/socket_watcher_test_util.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"

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
