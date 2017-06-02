// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_GCM_HANDLER_STUB_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_GCM_HANDLER_STUB_H_

#include <string>

#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"

namespace offline_pages {

// Stub for testing.
class PrefetchGCMHandlerStub : public PrefetchGCMHandler {
 public:
  PrefetchGCMHandlerStub();
  ~PrefetchGCMHandlerStub() override;

  gcm::GCMAppHandler* AsGCMAppHandler() override;
  std::string GetAppId() const override;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_GCM_HANDLER_STUB_H_
