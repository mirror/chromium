// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DISPATCHER_IMPL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DISPATCHER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/task_queue.h"
#include "components/version_info/channel.h"
#include "net/url_request/url_request_context_getter.h"

namespace offline_pages {
class GeneratePageBundleRequest;
class GetOperationRequest;
class PrefetchService;

class PrefetchDispatcherImpl : public PrefetchDispatcher {
 public:
  // Data bag containing all the embedder-specific information for network
  // requests.
  class NetworkParams {
   public:
    NetworkParams(net::URLRequestContextGetter* request_context,
                  const version_info::Channel& channel,
                  const std::string& user_agent);
    NetworkParams(NetworkParams&& other);
    ~NetworkParams();

    // This defines the storage partition for the network requests.
    scoped_refptr<net::URLRequestContextGetter> request_context;

    // Used to choose API keys.
    version_info::Channel channel;

    // Used server-side to help generate correctly formatted content.
    std::string user_agent;

   private:
    DISALLOW_COPY_AND_ASSIGN(NetworkParams);
  };

  explicit PrefetchDispatcherImpl(NetworkParams network_params);
  ~PrefetchDispatcherImpl() override;

  // PrefetchDispatcher implementation:
  void SetService(PrefetchService* service) override;
  void AddCandidatePrefetchURLs(
      const std::vector<PrefetchURL>& prefetch_urls) override;
  void RemoveAllUnprocessedPrefetchURLs(const std::string& name_space) override;
  void RemovePrefetchURLsByClientId(const ClientId& client_id) override;
  void BeginBackgroundTask(std::unique_ptr<ScopedBackgroundTask> task) override;
  void StopBackgroundTask() override;
  void GCMOperationCompletedMessageReceived(
      const std::string& operation_name) override;
  void RequestFinishBackgroundTaskForTest() override;

 private:
  friend class PrefetchDispatcherTest;

  void DisposeTask();
  void GotRegistrationId(const std::vector<PrefetchURL>& prefetch_urls,
                         const std::string& id,
                         instance_id::InstanceID::Result result);
  void DonePrefetchRequest(const std::string& request_name,
                           PrefetchRequestStatus status,
                           const std::string& operation_name,
                           const std::vector<RenderPageInfo>& pages);

  NetworkParams network_params_;

  std::unique_ptr<GeneratePageBundleRequest> page_bundle_request_;
  std::unique_ptr<GetOperationRequest> get_operation_request_;

  PrefetchService* service_;
  TaskQueue task_queue_;
  std::unique_ptr<ScopedBackgroundTask> task_;

  base::WeakPtrFactory<PrefetchDispatcherImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchDispatcherImpl);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DISPATCHER_IMPL_H_
