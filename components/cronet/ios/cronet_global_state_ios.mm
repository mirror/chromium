// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_global_state.h"

#import <Foundation/Foundation.h>

#include <utility>

#include "ios/web/public/global_state/ios_global_state.h"
#include "ios/web/public/global_state/ios_global_state_configuration.h"
#include "ios/web/public/user_agent.h"
#include "net/proxy/proxy_config_service.h"
#include "net/proxy/proxy_service.h"
#include "url/url_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace cronet {

void EnsureInitialized() {
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

void InitializeOnInitThread() {
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

bool OnInitThread() {
  if ([NSThread isMainThread])
    return true;
  return false;
}

std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner) {
  return net::ProxyService::CreateSystemProxyConfigService(io_task_runner);
}

std::unique_ptr<net::ProxyService> CreateProxyService(
    std::unique_ptr<net::ProxyConfigService> proxy_config_service,
    net::NetLog* net_log) {
  return nullptr;
}

// Creates default User-Agent request value, combining optional
// |partial_user_agent| with system-dependent values.
std::string CreateDefaultUserAgent(const std::string& partial_user_agent) {
  return web::BuildUserAgentFromProduct(partial_user_agent);
}

}  // namespace cronet
