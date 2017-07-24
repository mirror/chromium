// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_ARC_TRACING_AGENT_IMPL_H_
#define CONTENT_BROWSER_TRACING_ARC_TRACING_AGENT_IMPL_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "cc/base/ring_buffer.h"
#include "content/public/browser/arc_tracing_agent.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace base {
namespace trace_event {
class TraceConfig;
}  // namespace trace_event
}  // namespace base

namespace content {

namespace {
// Number of events for the ring buffer.
constexpr size_t kTraceEventBufferSize = 64000;
}  // namespace

class ArcTracingAgentImpl : public ArcTracingAgent {
 public:
  static ArcTracingAgentImpl* GetInstance();

  explicit ArcTracingAgentImpl(service_manager::Connector* connector);

  // base::trace_event::TracingAgent.
  // DEPRECATED: These will be deleted when tracing servicification is complete.
  // At that point, we will get rid of base::trace_event::TracingAgent and
  // tracing::mojom::Agent methods will be used, instead.
  std::string GetTracingAgentName() override;
  std::string GetTraceEventLabel() override;
  void StartAgentTracing(const base::trace_event::TraceConfig& trace_config,
                         const StartAgentTracingCallback& callback) override;
  void StopAgentTracing(const StopAgentTracingCallback& callback) override;

  // content::ArcTracingAent.
  void SetDelegate(Delegate* delegate) override;

 private:
  friend std::default_delete<ArcTracingAgentImpl>;

  // A helper class for reading trace data from the client side. We separate
  // this from |ArcTracingAgentImpl| to isolate the logic that runs on browser's
  // IO thread. All the functions in this class except for constructor,
  // destructor, and |GetWeakPtr| are expected to be run on browser's IO thread.
  class ArcTracingReader {
   public:
    using StopTracingCallback =
        base::Callback<void(const scoped_refptr<base::RefCountedString>&)>;

    ArcTracingReader();
    ~ArcTracingReader();

    void StartTracing(base::ScopedFD read_fd);
    void OnTraceDataAvailable();
    void StopTracing(const StopTracingCallback& callback);
    base::WeakPtr<ArcTracingReader> GetWeakPtr();

   private:
    base::ScopedFD read_fd_;
    std::unique_ptr<base::FileDescriptorWatcher::Controller> fd_watcher_;
    cc::RingBuffer<std::string, kTraceEventBufferSize> ring_buffer_;
    // NOTE: Weak pointers must be invalidated before all other member variables
    // so it must be the last member.
    base::WeakPtrFactory<ArcTracingReader> weak_ptr_factory_;

    DISALLOW_COPY_AND_ASSIGN(ArcTracingReader);
  };

  ~ArcTracingAgentImpl() override;

  // tracing::mojom::Agent. Called by Mojo internals on the UI thread.
  void StartTracing(const std::string& config,
                    const Agent::StartTracingCallback& callback) override;
  void StopAndFlush(tracing::mojom::RecorderPtr recorder) override;
  void RequestClockSyncMarker(
      const std::string& sync_id,
      const Agent::RequestClockSyncMarkerCallback& callback) override;
  void GetCategories(const Agent::GetCategoriesCallback& callback) override;
  void RequestBufferStatus(
      const Agent::RequestBufferStatusCallback& callback) override;

  void OnArcTracingStopped(const StopAgentTracingCallback& callback,
                           bool success);
  void OnTracingReaderStopped(
      const StopAgentTracingCallback& callback,
      const scoped_refptr<base::RefCountedString>& data);
  void RecorderProxy(const std::string& event_name,
                     const std::string& events_label,
                     const scoped_refptr<base::RefCountedString>& events);

  mojo::Binding<tracing::mojom::Agent> binding_;
  Delegate* delegate_ = nullptr;  // Owned by ArcServiceLauncher.
  // We use |reader_.GetWeakPtr()| when binding callbacks with its functions.
  // Notes that the weak pointer returned by it can only be deferenced or
  // invalided in the same task runner to avoid racing condition. The
  // destruction of |reader_| is also a source of invalidation. However, we're
  // lucky as we're using |ArcTracingAgentImpl| as a singleton, the
  // destruction is always performed after all MessageLoops are destroyed, and
  // thus there are no race conditions in such situation.
  ArcTracingReader reader_;
  bool is_stopping_ = false;
  // NOTE: Weak pointers must be invalidated before all other member variables
  // so it must be the last member.
  tracing::mojom::RecorderPtr recorder_;
  base::WeakPtrFactory<ArcTracingAgentImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcTracingAgentImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_ARC_TRACING_AGENT_IMPL
