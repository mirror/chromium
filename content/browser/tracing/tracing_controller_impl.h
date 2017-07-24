// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_TRACING_CONTROLLER_IMPL_H_
#define CONTENT_BROWSER_TRACING_TRACING_CONTROLLER_IMPL_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "content/browser/tracing/tracing_ui.h"
#include "content/common/content_export.h"
#include "content/public/browser/tracing_controller.h"
#include "content/public/browser/tracing_delegate.h"
#include "mojo/common/data_pipe_drainer.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"

namespace base {

namespace trace_event {
class TraceConfig;
}  // namespace trace_event

class DictionaryValue;
class RefCountedString;
}  // namespace base

namespace content {

class TracingUI;

class TracingControllerImpl : public TracingController,
                              public mojo::common::DataPipeDrainer::Client {
 public:
  // Create an endpoint for dumping the trace data to a callback.
  CONTENT_EXPORT static scoped_refptr<TraceDataEndpoint> CreateCallbackEndpoint(
      const base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                                base::RefCountedString*)>& callback);

  CONTENT_EXPORT static scoped_refptr<TraceDataEndpoint>
  CreateCompressedStringEndpoint(scoped_refptr<TraceDataEndpoint> endpoint);

  static TracingControllerImpl* GetInstance();

  // TracingController implementation.
  bool GetCategories(const GetCategoriesDoneCallback& callback) override;
  bool StartTracing(const base::trace_event::TraceConfig& trace_config,
                    const StartTracingDoneCallback& callback) override;
  bool StopTracing(const scoped_refptr<TraceDataEndpoint>& endpoint) override;
  bool GetTraceBufferUsage(
      const GetTraceBufferUsageCallback& callback) override;
  bool IsTracing() const override;

  void RegisterTracingUI(TracingUI* tracing_ui);
  void UnregisterTracingUI(TracingUI* tracing_ui);

 private:
  friend struct base::LazyInstanceTraitsBase<TracingControllerImpl>;

  TracingControllerImpl();
  ~TracingControllerImpl() override;
  void AddAgents();
  std::unique_ptr<base::DictionaryValue> GenerateMetadataDict() const;

  // mojo::Common::DataPipeDrainer::Client
  void OnDataAvailable(const void* data, size_t num_bytes) override;
  void OnDataComplete() override;

  void OnMetadataAvailable(std::unique_ptr<base::DictionaryValue> metadata);

  tracing::mojom::AgentRegistryPtr agent_registry_;
  tracing::mojom::CoordinatorPtr coordinator_;
  std::vector<std::unique_ptr<tracing::mojom::Agent>> agents_;
  std::unique_ptr<TracingDelegate> delegate_;
  std::unique_ptr<base::trace_event::TraceConfig> trace_config_;
  std::unique_ptr<mojo::common::DataPipeDrainer> drainer_;
  scoped_refptr<TraceDataEndpoint> trace_data_endpoint_;
  std::set<TracingUI*> tracing_uis_;

  DISALLOW_COPY_AND_ASSIGN(TracingControllerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_TRACING_CONTROLLER_IMPL_H_
