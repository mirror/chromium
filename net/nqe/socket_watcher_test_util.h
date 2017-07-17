// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_NQE_SOCKET_WATCHER_TEST_UTIL_H_
#define NET_NQE_SOCKET_WATCHER_TEST_UTIL_H_

#include "net/base/net_export.h"
#include "net/nqe/socket_watcher.h"

namespace net {

namespace nqe {

namespace internal {

class NET_EXPORT_PRIVATE TestSocketWatcher : public SocketWatcher {
 public:
  explicit TestSocketWatcher(const SocketWatcher& socket_watcher);
  ~TestSocketWatcher() override;

 private:
  bool AllowPrivateSockets() const override;

  DISALLOW_COPY_AND_ASSIGN(TestSocketWatcher);
};

}  // namespace internal

}  // namespace nqe

}  // namespace net

#endif  // NET_NQE_SOCKET_WATCHER_TEST_UTIL_H_
