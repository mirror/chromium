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

// An implementation of this interface is passed when constructing a
// TraceDataSink, and receives chunks of the final trace data as it's being
// constructed.
// Methods may be called from any thread.
class CONTENT_EXPORT TraceDataEndpoint
    : public base::RefCountedThreadSafe<TraceDataEndpoint> {
 public:
  virtual void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) {}
  virtual void ReceiveTraceEnd() {}

 protected:
  friend class base::RefCountedThreadSafe<TraceDataEndpoint>;
  virtual ~TraceDataEndpoint() {}
};

class TracingControllerImpl : public TracingController,
                              public mojo::common::DataPipeDrainer::Client {
 public:
  // Create an endpoint that may be supplied to any TraceDataSink to
  // dump the trace data to a callback.
  CONTENT_EXPORT static scoped_refptr<TraceDataEndpoint> CreateCallbackEndpoint(
      const base::Callback<void(base::RefCountedString*)>& callback);

  CONTENT_EXPORT static scoped_refptr<TraceDataSink> CreateCompressedStringSink(
      scoped_refptr<TraceDataEndpoint> endpoint);
  static scoped_refptr<TraceDataSink> CreateJSONSink(
      scoped_refptr<TraceDataEndpoint> endpoint);

  static TracingControllerImpl* GetInstance();

  // TracingController implementation.
  bool GetCategories(const GetCategoriesDoneCallback& callback) override;
  bool StartTracing(const base::trace_event::TraceConfig& trace_config,
                    const StartTracingDoneCallback& callback) override;
  bool StopTracing(const scoped_refptr<TraceDataSink>& sink) override;
  bool GetTraceBufferUsage(
      const GetTraceBufferUsageCallback& callback) override;
  bool IsTracing() const override;

  void RegisterTracingUI(TracingUI* tracing_ui);
  void UnregisterTracingUI(TracingUI* tracing_ui);

 private:
  friend struct base::LazyInstanceTraitsBase<TracingControllerImpl>;

  TracingControllerImpl();
  ~TracingControllerImpl() override;
  std::unique_ptr<base::DictionaryValue> GenerateTracingMetadataDict() const;

  // mojo::Common::DataPipeDrainer::Client
  void OnDataAvailable(const void* data, size_t num_bytes) override;
  void OnDataComplete() override;

  tracing::mojom::AgentRegistryPtr agent_registry_;
  tracing::mojom::CoordinatorPtr coordinator_;
  std::vector<std::unique_ptr<tracing::mojom::Agent>> agents_;
  bool is_tracing_ = false;
  std::unique_ptr<mojo::common::DataPipeDrainer> drainer_;
  scoped_refptr<TraceDataSink> trace_data_sink_;
  std::set<TracingUI*> tracing_uis_;

  DISALLOW_COPY_AND_ASSIGN(TracingControllerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_TRACING_CONTROLLER_IMPL_H_
