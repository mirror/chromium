// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PushManager_h
#define PushManager_h

#include "modules/ModulesExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "public/platform/modules/permissions/permission_status.mojom-blink.h"

namespace blink {

class ExceptionState;
class PushSubscriptionOptionsInit;
class ScriptPromise;
class ScriptPromiseResolver;
class ScriptState;
class ServiceWorkerRegistration;
struct WebPushSubscriptionOptions;

class MODULES_EXPORT PushManager final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PushManager* Create(ServiceWorkerRegistration* registration) {
    return new PushManager(registration);
  }

  // Web-exposed property:
  static Vector<String> supportedContentEncodings();

  // Web-exposed methods:
  ScriptPromise subscribe(ScriptState*,
                          const PushSubscriptionOptionsInit&,
                          ExceptionState&);
  ScriptPromise getSubscription(ScriptState*);
  ScriptPromise permissionState(ScriptState*,
                                const PushSubscriptionOptionsInit&,
                                ExceptionState&);

  void Trace(blink::Visitor*);

 private:
  explicit PushManager(ServiceWorkerRegistration*);

  // Second step of the PushManager.subscribe() process, where the
  // "applicationServerKey" for the new subscription should be known.
  void SubscribeWithApplicationServerKey(ScriptPromiseResolver*,
                                         const WebPushSubscriptionOptions&,
                                         const String& application_server_key);

  // Third step of the PushManager.subscribe() process, where permission to use
  // the Push API has been requested if necessary and possible.
  void SubscribeWithPermissionRequested(ScriptPromiseResolver*,
                                        const WebPushSubscriptionOptions&,
                                        mojom::blink::PermissionStatus);

  // Called when the permission state is known in response to a call to
  // PushManager.permissionState(). Resolves the promise.
  void GotPermissionStatus(ScriptPromiseResolver*,
                           mojom::blink::PermissionStatus);

  Member<ServiceWorkerRegistration> registration_;
};

}  // namespace blink

#endif  // PushManager_h
