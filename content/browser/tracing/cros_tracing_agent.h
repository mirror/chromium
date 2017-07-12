// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_CROS_TRACING_AGENT_H_
#define CONTENT_BROWSER_TRACING_CROS_TRACING_AGENT_H_

#include "base/memory/ref_counted_memory.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"

namespace content {

class CrOSTracingAgent : public tracing::mojom::Agent {
 private:
  CrOSTracingAgent();
  ~CrOSTracingAgent() override;

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

  void StartTracingCallbackProxy(const std::string& agent_name, bool success);
  void RecorderProxy(const std::string& event_name,
                     const std::string& events_label,
                     const scoped_refptr<base::RefCountedString>& events);

  chromeos::DebugDaemonClient* debug_daemon_ = nullptr;
  Agent::StartTracingCallback start_tracing_callback_;
  tracing::mojom::RecorderPtr recorder_;

  DISALLOW_COPY_AND_ASSIGN(CrOSTracingAgent);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_CROS_TRACING_AGENT_H_
