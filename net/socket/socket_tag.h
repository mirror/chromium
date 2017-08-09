// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SOCKET_TAG_H_
#define NET_SOCKET_SOCKET_TAG_H_

#include "build/build_config.h"
#include "net/socket/socket_descriptor.h"

#if defined(OS_ANDROID)
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // OS_ANDROID

namespace net {

class SocketTag {
 public:
#if defined(OS_ANDROID)
  SocketTag() : SocketTag(getuid(), DEFAULT_TAG) {}
  SocketTag(uid_t uid, int32_t tag) : uid_(uid), tag_(tag) {}
#else
  SocketTag() {}
#endif  // OS_ANDROID
  ~SocketTag() {}

  bool operator<(const SocketTag& other) const;
  bool operator==(const SocketTag& other) const;

  void Apply(SocketDescriptor socket) const;

 private:
#if defined(OS_ANDROID)
  static const int32_t DEFAULT_TAG = 0;
  uid_t uid_;
  int32_t tag_;
#endif  // OS_ANDROID
};

}  // namespace net

#endif  // NET_SOCKET_SOCKET_TAG_H_