// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_SERVICE_IMPL_H_
#define CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_SERVICE_IMPL_H_

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "content/browser/shared_worker/shared_worker_host.h"
#include "content/common/shared_worker/shared_worker_connector.mojom.h"
#include "content/common/shared_worker/shared_worker_factory.mojom.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/worker_service.h"

namespace content {

class MessagePort;
class SharedWorkerInstance;
class SharedWorkerHost;
class ResourceContext;
class WorkerServiceObserver;
class WorkerStoragePartitionId;

// The implementation of WorkerService. We try to place workers in an existing
// renderer process when possible.
class CONTENT_EXPORT SharedWorkerServiceImpl : public WorkerService {
 public:
  // Returns the SharedWorkerServiceImpl singleton.
  static SharedWorkerServiceImpl* GetInstance();

  // WorkerService implementation:
  bool TerminateWorker(int process_id, int route_id) override;
  std::vector<WorkerInfo> GetWorkers() override;
  void AddObserver(WorkerServiceObserver* observer) override;
  void RemoveObserver(WorkerServiceObserver* observer) override;

  // Creates the worker if necessary or connects to an already existing worker.
  void ConnectToWorker(
      int process_id,
      int frame_id,
      mojom::SharedWorkerInfoPtr info,
      mojom::SharedWorkerClientPtr client,
      blink::mojom::SharedWorkerCreationContextType creation_context_type,
      const MessagePort& message_port,
      ResourceContext* resource_context,
      const WorkerStoragePartitionId& partition_id);

  void DestroyHost(int process_id, int route_id);

 private:
  friend struct base::DefaultSingletonTraits<SharedWorkerServiceImpl>;
  friend class SharedWorkerServiceImplTest;

  using ReserveProcessFunc = bool (*)(int /* process_id */,
                                      int* /* route_id */,
                                      mojom::SharedWorkerFactoryPtr*);
  using ReleaseProcessFunc = void (*)(int /* process_id */);
  using IsProcessAliveFunc = bool (*)(int /* process_id */);

  using WorkerID = std::pair<int /* process_id */, int /* route_id */>;
  using WorkerHostMap = std::map<WorkerID, std::unique_ptr<SharedWorkerHost>>;

  SharedWorkerServiceImpl();
  ~SharedWorkerServiceImpl() override;

  void ResetForTesting();

  void CreateWorker(std::unique_ptr<SharedWorkerInstance> instance,
                    mojom::SharedWorkerClientPtr client,
                    int process_id,
                    int frame_id,
                    const MessagePort& message_port);

  // Returns nullptr if there is no such host.
  SharedWorkerHost* FindSharedWorkerHost(int process_id, int route_id);
  SharedWorkerHost* FindSharedWorkerHost(const SharedWorkerInstance& instance);

  // Function pointers to reserve & release the renderer process where the
  // worker will be instantiated on the UI thread. Tests may override these.
  // TODO(darin): Use an interface instead.
  static ReserveProcessFunc s_reserve_process_func_;
  static ReleaseProcessFunc s_release_process_func_;
  static IsProcessAliveFunc s_is_process_alive_func_;

  WorkerHostMap worker_hosts_;

  base::ObserverList<WorkerServiceObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(SharedWorkerServiceImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_SERVICE_IMPL_H_
