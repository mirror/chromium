// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_QUEUEING_CONNECTION_FILTER_H_
#define CONTENT_COMMON_QUEUEING_CONNECTION_FILTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/public/common/connection_filter.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace base {
class SequencedTaskRunner;
}

namespace content {

// A ConnectionFilter that queues all incoming bind interface requests until
// the calback returned by |GetREleaseCallback| is run.
class CONTENT_EXPORT QueueingConnectionFilter : public ConnectionFilter {
 public:
  QueueingConnectionFilter(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      std::unique_ptr<service_manager::BinderRegistry> registry);
  ~QueueingConnectionFilter() override;

  base::Closure GetReleaseCallback();

 private:
  struct PendingRequest {
    PendingRequest(const std::string& interface_name,
                   mojo::ScopedMessagePipeHandle interface_pipe);
    ~PendingRequest();

    std::string interface_name;
    mojo::ScopedMessagePipeHandle interface_pipe;
  };

  // ConnectionFilter:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle* interface_pipe,
                       service_manager::Connector* connector) override;

  void Release();

  THREAD_CHECKER(io_thread_checker_);
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  bool released_ = false;
  std::vector<std::unique_ptr<PendingRequest>> pending_requests_;
  std::unique_ptr<service_manager::BinderRegistry> registry_;

  base::WeakPtrFactory<QueueingConnectionFilter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(QueueingConnectionFilter);
};

}  // namespace content

#endif  // CONTENT_COMMON_QUEUEING_CONNECTION_FILTER_H_
