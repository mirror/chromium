// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_SERVICE_H_
#define SERVICES_METRICS_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "services/metrics/public/cpp/histogram_collector.h"
#include "services/metrics/public/cpp/histogram_provider.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/service_manager/public/cpp/service.h"

namespace metrics {

class Service : public service_manager::Service {
 public:
  ~Service() override;

  // Create an instance of the metrics service.
  static std::unique_ptr<service_manager::Service> Create();

 private:
  Service();

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  HistogramCollector histogram_collector_;
  HistogramProvider histogram_provider_;

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace metrics

#endif  // SERVICES_METRICS_SERVICE_H_
