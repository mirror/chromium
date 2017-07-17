// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/service.h"

#include "base/bind.h"
#include "services/metrics/public/cpp/histogram_collector.h"

namespace metrics {

Service::Service() {}

Service::~Service() {}

// static
std::unique_ptr<service_manager::Service> Service::Create() {
  return std::unique_ptr<Service>(new Service());
}

void Service::OnStart() {
  // Register bindings for interfaces hosted by this service.
  registry_.AddInterface<mojom::HistogramCollector>(base::BindRepeating(
      &HistogramCollector::Bind, base::Unretained(&histogram_collector_)));
  registry_.AddInterface<mojom::HistogramProvider>(base::BindRepeating(
      &HistogramProvider::Bind, base::Unretained(&histogram_provider_)));
}

void Service::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(source_info, interface_name,
                          std::move(interface_pipe));
}

}  // namespace metrics
