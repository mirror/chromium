// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_TRACING_COORDINATOR_H_
#define SERVICES_RESOURCE_COORDINATOR_TRACING_COORDINATOR_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"
#include "services/resource_coordinator/tracing/agent_registry.h"
#include "services/resource_coordinator/tracing/recorder.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace tracing {

class Coordinator : public mojom::Coordinator {
 public:
  static Coordinator* GetInstance();

  Coordinator();

  void BindCoordinatorRequest(
      const service_manager::BindSourceInfo& source_info,
      mojom::CoordinatorRequest request);

 private:
  friend std::default_delete<Coordinator>;
  friend class CoordinatorTest;  // For testing.

  ~Coordinator() override;

  // mojom::Coordinator
  void StartTracing(const std::string& config,
                    const StartTracingCallback& callback) override;
  void StopAndFlush(mojo::ScopedDataPipeProducerHandle stream) override;
  void IsTracing(const IsTracingCallback& callback) override;
  void RequestBufferUsage(const RequestBufferUsageCallback& callback) override;
  void GetCategories(const GetCategoriesCallback& callback) override;

  // Internal methods for collecting events from agents.
  void SendStartTracingToAgent(AgentRegistry::AgentEntry* agent_entry);
  void OnTracingStarted(AgentRegistry::AgentEntry* agent_entry);
  void StopAndFlushInternal();
  void OnRecorderDataChange(const std::string& label);
  bool StreamEventsForCurrentLabel();
  void StreamMetadata();
  void OnFlushDone();

  void OnRequestBufferStatusResponse(AgentRegistry::AgentEntry* agent_entry,
                                     uint32_t capacity,
                                     uint32_t count);

  void OnGetCategoriesResponse(AgentRegistry::AgentEntry* agent_entry,
                               const std::string& categories);

  mojo::Binding<mojom::Coordinator> binding_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;
  AgentRegistry* agent_registry_;
  std::string config_;
  std::map<std::string, std::set<std::unique_ptr<Recorder>>> recorders_;
  bool is_tracing_ = false;

  // The stream to which trace events from different agents should be
  // serialized, eventually. This is set when tracing is stopped.
  mojo::ScopedDataPipeProducerHandle stream_;
  // If |streaming_label_| is not empty, it shows the label for which we are
  // writing chunks to the output stream.
  std::string streaming_label_;
  bool stream_header_written_ = false;
  bool json_field_name_written_ = false;
  size_t pending_start_tracing_count_ = 0;
  StartTracingCallback start_tracing_callback_;

  // For computing trace buffer usage.
  float maximum_trace_buffer_usage_ = 0;
  uint32_t approximate_event_count_ = 0;
  size_t pending_request_buffer_status_count_ = 0;
  RequestBufferUsageCallback request_buffer_usage_callback_;

  // For getting categories.
  std::set<std::string> category_set_;
  size_t pending_get_categories_count_ = 0;
  GetCategoriesCallback get_categories_callback_;

  DISALLOW_COPY_AND_ASSIGN(Coordinator);
};

}  // namespace tracing
#endif  // SERVICES_RESOURCE_COORDINATOR_TRACING_COORDINATOR_H_
