// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/ios/ios_global_state_cronet_configuration.h"

namespace {

ios_global_state::IOSGlobalStateCronetConfiguration* g_cronet_configuration =
    nullptr;

}  // namespace

namespace ios_global_state {

scoped_refptr<base::SingleThreadTaskRunner> GetIOThreadTaskRunner() {
  if (!g_cronet_configuration) {
    g_cronet_configuration =
        new ios_global_state::IOSGlobalStateCronetConfiguration();
  }
  return g_cronet_configuration->GetIOThreadTaskRunner();
}

}  // namespace ios_global_state
