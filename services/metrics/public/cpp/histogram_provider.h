// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_PROVIDER_H_
#define SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/metrics/public/interfaces/histogram.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace metrics {

// This class provides histograms to remote services.
class HistogramProvider : public mojom::HistogramProvider {
 public:
  HistogramProvider();
  ~HistogramProvider() override;

  // Bind this interface to a request.
  void Bind(const service_manager::BindSourceInfo& source_info,
            mojom::HistogramProviderRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // mojom::HistogramProvider implementation.
  void GetHistogram(const std::string& name,
                    GetHistogramCallback callback) override;

 private:
  mojo::BindingSet<mojom::HistogramProvider> bindings_;

  DISALLOW_COPY_AND_ASSIGN(HistogramProvider);
};

}  // namespace metrics

#endif  // SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_PROVIDER_H_
