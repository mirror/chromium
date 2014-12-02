// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_PUSH_MESSAGING_PUSH_MANAGER_H_
#define CONTENT_CHILD_PUSH_MESSAGING_PUSH_MANAGER_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "content/child/push_messaging/push_dispatcher.h"
#include "content/child/worker_task_runner.h"
#include "third_party/WebKit/public/platform/WebPushProvider.h"

class GURL;

namespace content {

class ThreadSafeSender;

class PushProvider : public blink::WebPushProvider,
                     public WorkerTaskRunner::Observer {
 public:
  ~PushProvider() override;

  // The |thread_safe_sender| and |push_dispatcher| are used if calling this
  // leads to construction.
  static PushProvider* ThreadSpecificInstance(
      ThreadSafeSender* thread_safe_sender,
      PushDispatcher* push_dispatcher);

  // WorkerTaskRunner::Observer implementation.
  void OnWorkerRunLoopStopped() override;

  // blink::WebPushProvider implementation.
  void registerPushMessaging(blink::WebServiceWorkerRegistration*,
                             blink::WebPushRegistrationCallbacks*) override;
  void getPermissionStatus(blink::WebServiceWorkerRegistration*,
                           blink::WebPushPermissionStatusCallbacks*) override;

  // Called by the PushDispatcher.
  bool OnMessageReceived(const IPC::Message& message);

 private:
  PushProvider(ThreadSafeSender* thread_safe_sender,
               PushDispatcher* push_dispatcher);

  // IPC message handlers.
  void OnRegisterFromWorkerSuccess(int request_id,
                                   const GURL& endpoint,
                                   const std::string& registration_id);
  void OnRegisterFromWorkerError(int request_id, PushRegistrationStatus status);
  void OnGetPermissionStatusSuccess(int request_id,
                                    blink::WebPushPermissionStatus status);
  void OnGetPermissionStatusError(int request_id);

  scoped_refptr<ThreadSafeSender> thread_safe_sender_;
  scoped_refptr<PushDispatcher> push_dispatcher_;

  // Stores the registration callbacks with their request ids. This class owns
  // the callbacks.
  std::map<int, blink::WebPushRegistrationCallbacks*> registration_callbacks_;

  // Stores the permission status callbacks with their request ids. This class
  // owns the callbacks.
  std::map<int, blink::WebPushPermissionStatusCallbacks*>
      permission_status_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(PushProvider);
};

}  // namespace content

#endif  // CONTENT_CHILD_PUSH_MESSAGING_PUSH_MANAGER_H_
