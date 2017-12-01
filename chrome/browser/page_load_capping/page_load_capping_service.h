// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_SERVICE_H_
#define CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class FilePath;
}

namespace page_load_capping {
class PageLoadCappingIOData;
class PageLoadCappingUIService;
}  // namespace page_load_capping

// Keyed service that owns a page_load_capping::PageLoadCappingUIService.
// PageLoadCappingService lives on the UI thread.
class PageLoadCappingService : public KeyedService {
 public:
  PageLoadCappingService();
  ~PageLoadCappingService() override;

  DISALLOW_COPY_AND_ASSIGN(PageLoadCappingService);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_SERVICE_H_
