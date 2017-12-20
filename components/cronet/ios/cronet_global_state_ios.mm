// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_global_state.h"

#import <Foundation/Foundation.h>

#include <utility>

#include "ios/web/public/global_state/ios_global_state.h"
#include "ios/web/public/global_state/ios_global_state_configuration.h"
#include "url/url_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace cronet {

// static
void CronetGlobalState::InitializeOnInitThread() {
  // This method must be called once from the main thread.
  DCHECK_EQ([NSThread currentThread], [NSThread mainThread]);

  ios_global_state::CreateParams create_params;
  create_params.install_at_exit_manager = true;
  ios_global_state::Create(create_params);
  ios_global_state::StartTaskScheduler(/*init_params=*/nullptr);

  url::Initialize();

  ios_global_state::BuildMessageLoop();
  ios_global_state::CreateNetworkChangeNotifier();
}

// static
bool CronetGlobalState::IsOnInitThread() {
  if ([NSThread isMainThread])
    return true;
  return false;
}

// static
void CronetGlobalState::ConfigureProxyConfigService(
    net::ProxyConfigService* service) {}

// static
void CronetGlobalState::EnsureInitialized() {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    if (![NSThread isMainThread]) {
      dispatch_sync(dispatch_get_main_queue(), ^(void) {
        InitializeOnInitThread();
      });
    } else {
      InitializeOnInitThread();
    }
  });
}

}  // namespace cronet
