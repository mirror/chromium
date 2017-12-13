// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PushMessagingBridge_h
#define PushMessagingBridge_h

#include "modules/serviceworkers/ServiceWorkerRegistration.h"
#include "platform/Supplementable.h"
#include "platform/heap/GarbageCollected.h"
#include "public/platform/modules/permissions/permission.mojom-blink.h"
#include "public/platform/modules/permissions/permission_status.mojom-blink.h"

namespace blink {

// The bridge is responsible for establishing and maintaining the Mojo
// connection to the permission service. It's keyed on an active Service Worker
// Registration.
//
// TODO(peter): Use the PushMessaging Mojo service directly from here.
class PushMessagingBridge final
    : public GarbageCollectedFinalized<PushMessagingBridge>,
      public Supplement<ServiceWorkerRegistration> {
  USING_GARBAGE_COLLECTED_MIXIN(PushMessagingBridge);
  WTF_MAKE_NONCOPYABLE(PushMessagingBridge);

 public:
  static PushMessagingBridge* From(ServiceWorkerRegistration*);
  static const char* SupplementName();

  virtual ~PushMessagingBridge();

  // Asynchronously requests permission to use the Push API for the current
  // origin. This will return the permission state when called in a worker.
  void RequestPermission(
      base::OnceCallback<void(mojom::blink::PermissionStatus)>);

  // Asynchronously determines the permission state for the current origin.
  void GetPermissionStatus(
      base::OnceCallback<void(mojom::blink::PermissionStatus)>);

  // Asynchronously reads the "gcm_sender_id" value from the manifest. Must only
  // be used by documents, as workers cannot have a manifest.
  void GetApplicationServerKeyFromManifest(
      base::OnceCallback<void(const String&)>);

 private:
  explicit PushMessagingBridge(ServiceWorkerRegistration&);

  // Called when the application server key may have been loaded from the
  // manifest. |application_server_key| will be the empty string when not found.
  void GotApplicationServerKeyFromManifest(
      base::OnceCallback<void(const String&)>,
      const WebString& application_server_key);

  // Gets the execution context from the supplemented ServiceWorkerRegistration.
  ExecutionContext* GetExecutionContext();

  mojom::blink::PermissionServicePtr permission_service_;
};

}  // namespace blink

#endif  // PushMessagingBridge_h
