// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_CRONET_GLOBAL_STATE_H_
#define COMPONENTS_CRONET_CRONET_GLOBAL_STATE_H_

#include <string>

#include "base/lazy_instance.h"
#include "base/macros.h"

namespace net {
class ProxyConfigService;
}  // namespace net

namespace cronet {

class CronetGlobalState {
 public:
  // Initialize Cronet environment globals. May be called multiple times
  // from any thread.
  static void EnsureInitialized();

  std::string GetDefaultUserAgent();

  static bool IsOnInitThread();

  static void ConfigureProxyConfigService(net::ProxyConfigService* service);

 private:
  // Initialize Cronet environment globals. Must be called only once on the
  // main thread.
  static void InitializeOnInitThread();

  DISALLOW_COPY_AND_ASSIGN(CronetGlobalState);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_CRONET_GLOBAL_STATE_H_
