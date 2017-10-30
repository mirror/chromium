// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_optimization_guide.h"

#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request.h"

namespace previews {

Hints::Hints() {}

Hints::~Hints() {}

PreviewsOptimizationGuide::PreviewsOptimizationGuide() {}

PreviewsOptimizationGuide::~PreviewsOptimizationGuide() {}

void PreviewsOptimizationGuide::SwapHints(const Hints& hints) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // TODO(crbug.com/777892): Implement this.
}

bool PreviewsOptimizationGuide::IsWhitelisted(const net::URLRequest& request,
                                              PreviewsType type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // TODO(crbug.com/777892): Implement this.
  return false;
}

}  // namespace previews
