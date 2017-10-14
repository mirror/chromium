// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_HOST_MAP_H_
#define CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_HOST_MAP_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "content/public/browser/worker_service.h"

namespace content {
class SharedWorkerHost;
class SharedWorkerInstance;

using SharedWorkerID = std::pair<int /* process_id */, int /* route_id */>;

class SharedWorkerHostMap {
 public:
  SharedWorkerHostMap();
  ~SharedWorkerHostMap();

  void Add(SharedWorkerID id, std::unique_ptr<SharedWorkerHost> host);
  void Remove(SharedWorkerID id);

  SharedWorkerHost* Find(SharedWorkerID id);
  SharedWorkerHost* FindAvailable(const SharedWorkerInstance& instance);

  void GetWorkers(std::vector<WorkerService::WorkerInfo>* results);

  void TerminateAllWorkersForTesting(base::OnceClosure callback);
  void ResetForTesting();

 private:
  using HostMap = std::map<SharedWorkerID, std::unique_ptr<SharedWorkerHost>>;
  HostMap hosts_;
  base::OnceClosure terminate_all_workers_callback_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_HOST_MAP_H_
