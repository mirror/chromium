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
  SocketTag() : SocketTag(UNSET_UID, UNSET_TAG) {}
  SocketTag(uid_t uid, int32_t tag) : uid_(uid), tag_(tag) {}
#else
  SocketTag() {}
#endif  // OS_ANDROID
  ~SocketTag() {}

  bool operator<(const SocketTag& other) const;
  bool operator==(const SocketTag& other) const;
  bool operator!=(const SocketTag& other) { return !(*this == other); }

  void Apply(SocketDescriptor socket) const;

#if defined(OS_ANDROID)
  // These values match those in Android:
  // http://androidxref.com/7.1.1_r6/xref/frameworks/base/core/java/android/net/TrafficStats.java#191
  // http://androidxref.com/7.1.1_r6/xref/frameworks/base/core/java/android/net/TrafficStats.java#221
  static const uid_t UNSET_UID = -1;
  static const int32_t UNSET_TAG = -1;

 private:
  uid_t uid_;
  int32_t tag_;
#endif  // OS_ANDROID
};

}  // namespace net

#endif  // NET_SOCKET_SOCKET_TAG_H_