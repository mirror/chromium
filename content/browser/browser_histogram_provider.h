// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSER_HISTOGRAM_PROVIDER_H_
#define CONTENT_BROWSER_BROWSER_HISTOGRAM_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "content/common/histogram.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace content {

// This class sends and receives histogram messages in the browser process.
class BrowserHistogramProvider : public mojom::BrowserHistogramProvider {
 public:
  BrowserHistogramProvider();
  ~BrowserHistogramProvider() override;

  // Bind this interface to a request.
  void Bind(const service_manager::BindSourceInfo& source_info,
            mojom::BrowserHistogramProviderRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // mojom::BrowserHistogramProvider implementation.
  void GetHistogram(const std::string& name,
                    GetHistogramCallback callback) override;

 private:
  mojo::BindingSet<mojom::BrowserHistogramProvider> bindings_;

  DISALLOW_COPY_AND_ASSIGN(BrowserHistogramProvider);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BROWSER_HISTOGRAM_PROVIDER_H_
