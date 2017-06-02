// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_gcm_handler_stub.h"

namespace offline_pages {

PrefetchGCMHandlerStub::PrefetchGCMHandlerStub() = default;
PrefetchGCMHandlerStub::~PrefetchGCMHandlerStub() = default;

gcm::GCMAppHandler* PrefetchGCMHandlerStub::AsGCMAppHandler() {
  return nullptr;
}

std::string PrefetchGCMHandlerStub::GetAppId() const {
  return "com.google.test.PrefetchAppId";
}

}  // namespace offline_pages
