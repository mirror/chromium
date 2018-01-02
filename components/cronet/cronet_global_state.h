// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_CRONET_GLOBAL_STATE_H_
#define COMPONENTS_CRONET_CRONET_GLOBAL_STATE_H_

#include <string>
#include "base/sequenced_task_runner.h"

namespace net {
class ProxyConfigService;
}  // namespace net

namespace cronet {
namespace global_state {

void EnsureInitialized();

void InitializeOnInitThread();

bool IsOnInitThread();

// Creates a proxy config service appropriate for this platform that fetches the
// system proxy settings.
std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner);

}  // namespace global_state
}  // namespace cronet

#endif  // COMPONENTS_CRONET_CRONET_GLOBAL_STATE_H_
