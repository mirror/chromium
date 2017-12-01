// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_capping/page_load_capping_service.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/common/chrome_constants.h"
#include "content/public/browser/browser_thread.h"

namespace {

PageLoadCappingService::PageLoadCappingService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

PageLoadCappingService::~PageLoadCappingService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

PageLoadCappingService::ObservePageLoad(content::WebContents* web_contents) {}