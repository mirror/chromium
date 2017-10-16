// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSER_HISTOGRAM_FETCHER_IMPL_H_
#define CONTENT_BROWSER_BROWSER_HISTOGRAM_FETCHER_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "content/common/histogram_fetcher.mojom.h"
#include "content/public/common/process_type.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace content {

// This class sends and receives histogram messages in the browser process.
class BrowserHistogramFetcherImpl
    : public content::mojom::BrowserHistogramFetcher {
 public:
  BrowserHistogramFetcherImpl();
  ~BrowserHistogramFetcherImpl() override;

  static void Create(content::mojom::BrowserHistogramFetcherRequest,
                     const service_manager::BindSourceInfo&);

 private:
  using BrowserHistogramCallback =
      content::mojom::BrowserHistogramFetcher::GetBrowserHistogramCallback;
  // Message handlers.
  void GetBrowserHistogram(const std::string& name,
                           BrowserHistogramCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(BrowserHistogramFetcherImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BROWSER_HISTOGRAM_FETCHER_IMPL_H_
