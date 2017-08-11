// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/browser_process.h"
#include "components/ukm/ukm_interface.h"
#include "content/public/browser/browser_thread.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace metrics {

namespace {

class MetricsMojoService : public service_manager::Service {
 public:
  MetricsMojoService() {
    registry_.AddInterface(base::Bind(&ukm::UkmInterface::Create,
                                      g_browser_process->ukm_recorder()),
                           content::BrowserThread::GetTaskRunnerForThread(
                               content::BrowserThread::IO));
  }

  ~MetricsMojoService() final {}

 private:
  // service_manager::Service implementation.
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle handle) override {
    registry_.BindInterface(interface_name, std::move(handle));
  }

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(MetricsMojoService);
};

}  // namespace

std::unique_ptr<service_manager::Service> CreateMetricsService() {
  return base::MakeUnique<MetricsMojoService>();
}

}  // namespace metrics
