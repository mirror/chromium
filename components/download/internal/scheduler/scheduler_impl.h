// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_SCHEDULER_IMPL_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_SCHEDULER_IMPL_H_

#include "components/download/internal/scheduler/scheduler.h"

#include <vector>

#include "base/macros.h"
#include "components/download/internal/entry.h"

namespace download {

// Scheduler implementation that
// 1. Routes download requests to platform task scheduler.
// 2. Polls the next entry to be processed by the service.
//
// Provides load balancing between download clients using the service.
class SchedulerImpl : public Scheduler {
 public:
  SchedulerImpl(PlatformTaskScheduler* platform_scheduler,
                const std::vector<int>& clients);
  ~SchedulerImpl();

  // Scheduler implementation.
  void Reschedule(const Model::EntryList& entries) override;
  Entry* Next(const Model::EntryList& entries,
              const DeviceStatus& device_status) override;

 private:
  // Get the next download with a specified client id.
  Entry* GetNextByClientId(int client,
                           const Model::EntryList& entries,
                           const DeviceStatus& device_status);

  // Used to create platform dependent background tasks.
  PlatformTaskScheduler* platform_scheduler_;

  // List of all download client id, used in load balancing.
  // Downloads will be delivered to clients with incremental order based on
  // the index of this list.
  std::vector<int> download_clients_;

  // The index of the current client.
  // See |download_clients_|.
  int current_client_index_;

  DISALLOW_COPY_AND_ASSIGN(SchedulerImpl);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_SCHEDULER_IMPL_H_
