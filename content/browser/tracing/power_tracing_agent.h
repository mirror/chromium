// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_POWER_TRACING_AGENT_H_
#define CONTENT_BROWSER_TRACING_POWER_TRACING_AGENT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/threading/thread.h"
#include "base/trace_event/tracing_agent.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"
#include "tools/battor_agent/battor_agent.h"
#include "tools/battor_agent/battor_error.h"

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace content {

using tracing::mojom::Agent;

class PowerTracingAgent : public Agent,
                          public base::trace_event::TracingAgent,
                          public battor::BattOrAgent::Listener {
 public:
  // Retrieve the singleton instance.
  static PowerTracingAgent* GetInstance();

  explicit PowerTracingAgent(service_manager::Connector* connector);

  // base::trace_event::TracingAgent.
  // DEPRECATED: These will be deleted when tracing servicification is complete.
  // At that point, we will get rid of base::trace_event::TracingAgent and
  // tracing::mojom::Agent methods will be used, instead.
  void StartAgentTracing(const base::trace_event::TraceConfig& trace_config,
                         const StartAgentTracingCallback& callback) override;
  void StopAgentTracing(const StopAgentTracingCallback& callback) override;
  void RecordClockSyncMarker(
      const std::string& sync_id,
      const RecordClockSyncMarkerCallback& callback) override;
  bool SupportsExplicitClockSync() override;
  std::string GetTracingAgentName() override;
  std::string GetTraceEventLabel() override;

  // BattOrAgent::Listener implementation.
  void OnStartTracingComplete(battor::BattOrError error) override;
  void OnStopTracingComplete(const std::string& trace,
                             battor::BattOrError error) override;
  void OnRecordClockSyncMarkerComplete(battor::BattOrError error) override;
  void OnGetFirmwareGitHashComplete(const std::string& version,
                                    battor::BattOrError error) override;

 private:
  // This allows constructor and destructor to be private and usable only
  // by the Singleton class.
  friend struct base::DefaultSingletonTraits<PowerTracingAgent>;
  friend std::default_delete<PowerTracingAgent>;

  PowerTracingAgent();
  ~PowerTracingAgent() override;

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

  void FindBattOrOnFileThread(const Agent::StartTracingCallback& callback);
  void StartTracingOnIOThread(const std::string& path,
                              const Agent::StartTracingCallback& callback);
  void StopAndFlushOnIOThread(const StopAgentTracingCallback& callback);
  void RecorderProxy(const std::string& agent_name,
                     const std::string& events_label,
                     const scoped_refptr<base::RefCountedString>& events);

  void RequestClockSyncMarkerOnIOThread(
      const std::string& sync_id,
      const Agent::RequestClockSyncMarkerCallback& callback);

  // Returns the path of a BattOr (e.g. /dev/ttyUSB0), or an empty string if
  // none are found.
  std::string GetBattOrPath();

  // All interactions with the BattOrAgent (after construction) must happen on
  // the IO thread.
  std::unique_ptr<battor::BattOrAgent, BrowserThread::DeleteOnIOThread>
      battor_agent_;

  Agent::StartTracingCallback start_tracing_callback_;
  StopAgentTracingCallback stop_tracing_callback_;
  tracing::mojom::RecorderPtr recorder_;
  base::TimeTicks request_clock_sync_marker_start_time_;
  Agent::RequestClockSyncMarkerCallback request_clock_sync_marker_callback_;

  mojo::Binding<tracing::mojom::Agent> binding_;

  DISALLOW_COPY_AND_ASSIGN(PowerTracingAgent);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_POWER_TRACING_AGENT_H_
