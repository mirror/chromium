// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_EMBEDDED_INSTANCE_MANAGER_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_EMBEDDED_INSTANCE_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_checker.h"
#include "services/service_manager/embedder/embedded_service_info.h"
#include "services/service_manager/embedder/service_manager_embedder_export.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace base {
class SequencedTaskRunner;
class SingleThreadTaskRunner;
class Thread;

enum class ThreadPriority : int;
}  // namespace base

namespace service_manager {

// Creates and keeps ownership of a ServiceContext for each time the service is
// bound. Creates a dedicated thread for the ServiceContext instances to
// operate on if requested via |EmbeddedServiceInfo.use_own_thread|.
// Invokes a given |quit_closure| whenever the connection to the last
// ServiceContext is lost or closed via invocation of ShutDown(). Calling any
// public method after invocation of ShutDown() gives undefined behavior.
// In order to allow for a clean shutdown, the task runner from which
// ShutDownAsync() is called must keep serving tasks at least until
// |quit_closure| has been invoked.
class SERVICE_MANAGER_EMBEDDER_EXPORT EmbeddedInstanceManager
    : public base::RefCountedThreadSafe<EmbeddedInstanceManager> {
 public:
  EmbeddedInstanceManager(const base::StringPiece& name,
                          const EmbeddedServiceInfo& info,
                          base::RepeatingClosure quit_closure);

  // The given |observer_cb| is invoked whenever potentially asynchronous
  // handling of one of the following external events is complete:
  // * Call to ShutDownAsync()
  // * Connection to a ServiceContext was lost
  void SetEventHandlingCompleteObserver(base::RepeatingClosure observer_cb);

  void BindServiceRequest(service_manager::mojom::ServiceRequest request);
  void ShutDownAsync();

 private:
  friend class base::RefCountedThreadSafe<EmbeddedInstanceManager>;

  ~EmbeddedInstanceManager();

  void BindServiceRequestOnServiceSequence(
      service_manager::mojom::ServiceRequest request);
  void OnInstanceLost(int instance_id);
  void ShutDownOnServiceSequence();
  void RunOrPostQuitClosureOnOwnerTaskRunner();
  void DecreaseServiceRequestCount();
  void SetServiceRequestCountToZero();
  void OnServiceRequestCountChangedToZero();
  void OnEventHandlingComplete();

  const std::string name_;
  const EmbeddedServiceInfo::ServiceFactory factory_callback_;
  const bool use_dedicated_thread_;
  base::MessageLoop::Type message_loop_type_;
  base::ThreadPriority thread_priority_;
  base::RepeatingClosure quit_closure_;
  const scoped_refptr<base::SingleThreadTaskRunner> owner_task_runner_;
  base::RepeatingClosure optional_event_handling_complete_observer_cb_;

  // These fields must only be accessed from |owner_task_runner_|.
  std::unique_ptr<base::Thread> optional_dedicated_service_thread_;
  scoped_refptr<base::SequencedTaskRunner> service_task_runner_;
  int active_and_pending_service_request_count_;
  bool shutdown_has_been_called_;

  // These fields must only be accessed from |service_task_runner_|, except in
  // the destructor which may run on either |owner_task_runner_| or
  // |service_task_runner_|.
  int next_instance_id_ = 0;
  std::map<int, std::unique_ptr<service_manager::ServiceContext>>
      id_to_context_map_;

  DISALLOW_COPY_AND_ASSIGN(EmbeddedInstanceManager);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_EMBEDDED_INSTANCE_MANAGER_H_
