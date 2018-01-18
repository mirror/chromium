// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_RESOURCE_SCHEDULER_CLIENT_H_
#define CONTENT_NETWORK_RESOURCE_SCHEDULER_CLIENT_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/network/resource_scheduler.h"
#include "net/base/request_priority.h"

namespace net {
class URLRequest;
}

namespace content {

class NetworkContext;

// ResourceSchedulerClient represents ResourceScheduler::Client. At this
// moment it uses two integers (child_id and route_id) to specify a client
// for compatibility with content/browser/loader users, but this should be
// changed to a less error-prone interface once Network Service is launched.
// A ResourceSchedulerClient instance can be shared, but it must not outlive
// the associated NetworkContext.
class CONTENT_EXPORT ResourceSchedulerClient final
    : public base::RefCounted<ResourceSchedulerClient> {
 public:
  ResourceSchedulerClient(int child_id, int route_id, NetworkContext* context);

  std::unique_ptr<ResourceScheduler::ScheduledResourceRequest> ScheduleRequest(
      bool is_async,
      net::URLRequest* url_request);
  void ReprioritizeRequest(net::URLRequest* request,
                           net::RequestPriority new_priority,
                           int intra_priority_value);
  void OnReceivedSpdyProxiedHttpResponse();

 private:
  friend class base::RefCounted<ResourceSchedulerClient>;
  ~ResourceSchedulerClient();

  const int child_id_;
  const int route_id_;
  NetworkContext* context_;
  bool has_actually_created_client_ = false;

  DISALLOW_COPY_AND_ASSIGN(ResourceSchedulerClient);
};
}  // namespace content

#endif  // CONTENT_NETWORK_RESOURCE_SCHEDULER_CLIENT_H_
