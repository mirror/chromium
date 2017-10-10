// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/net/network_change_notifier_factory_cast.h"

#include "chromecast/net/net_util_cast.h"

#if defined(OS_LINUX)
#include "net/base/network_change_notifier_linux.h"
#endif

namespace chromecast {

net::NetworkChangeNotifier* NetworkChangeNotifierFactoryCast::CreateInstance() {
#if defined(OS_LINUX)
  // Caller assumes ownership.
  return new net::NetworkChangeNotifierLinux(GetIgnoredInterfaces());
#else
  return nullptr;
#endif
}

NetworkChangeNotifierFactoryCast::~NetworkChangeNotifierFactoryCast() {
}

}  // namespace chromecast
