// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_global_state.h"

#include <utility>

#include "url/url_util.h"

namespace cronet {

// static
void CronetGlobalState::InitializeOnInitThread() {
  // This method must be called once from the main thread.
  url::Initialize();
}

// static
void CronetGlobalState::EnsureInitialized() {}

}  // namespace cronet
