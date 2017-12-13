// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/push_messaging/PushManager.h"

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/Frame.h"
#include "core/frame/LocalFrame.h"
#include "modules/push_messaging/PushError.h"
#include "modules/push_messaging/PushMessagingBridge.h"
#include "modules/push_messaging/PushSubscription.h"
#include "modules/push_messaging/PushSubscriptionCallbacks.h"
#include "modules/push_messaging/PushSubscriptionOptions.h"
#include "modules/push_messaging/PushSubscriptionOptionsInit.h"
#include "modules/serviceworkers/ServiceWorkerRegistration.h"
#include "platform/bindings/ScriptState.h"
#include "platform/wtf/Assertions.h"
#include "public/platform/Platform.h"
#include "public/platform/modules/push_messaging/WebPushProvider.h"
#include "public/platform/modules/push_messaging/WebPushSubscriptionOptions.h"

namespace blink {
namespace {

// Error message to include when attempting to subscribe without passing an
// applicationServerKey, either in the subscribe() call or through a manifest.
const char kMissingApplicationServerKeyMessage[] =
    "Registration failed - missing applicationServerKey, and "
    "gcm_sender_id not found in manifest";

// Error message to include when attempting to subscribe without permission.
const char kPermissionDeniedMessage[] =
    "Registration failed - permission denied";

// Error message to explain that the userVisibleOnly flag must be set.
const char kUserVisibleOnlyRequired[] =
    "Push subscriptions that don't enable userVisibleOnly are not supported.";

// Returns the Push API's permission value corresponding to the given |status|.
// https://w3c.github.io/push-api/#dom-pushpermissionstate
String PermissionStatusToString(mojom::blink::PermissionStatus status) {
  switch (status) {
    case mojom::blink::PermissionStatus::GRANTED:
      return "granted";
    case mojom::blink::PermissionStatus::DENIED:
      return "denied";
    case mojom::blink::PermissionStatus::ASK:
      return "prompt";
  }

  NOTREACHED();
  return "denied";
}

WebPushProvider* PushProvider() {
  WebPushProvider* web_push_provider = Platform::Current()->PushProvider();
  DCHECK(web_push_provider);
  return web_push_provider;
}

}  // namespace

PushManager::PushManager(ServiceWorkerRegistration* registration)
    : registration_(registration) {
  DCHECK(registration);
}

// static
Vector<String> PushManager::supportedContentEncodings() {
  return Vector<String>({"aes128gcm", "aesgcm"});
}

ScriptPromise PushManager::subscribe(ScriptState* script_state,
                                     const PushSubscriptionOptionsInit& options,
                                     ExceptionState& exception_state) {
  if (!registration_->active())
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kAbortError,
                             "Subscription failed - no active Service Worker"));

  const WebPushSubscriptionOptions& web_options =
      PushSubscriptionOptions::ToWeb(options, exception_state);
  if (exception_state.HadException())
    return ScriptPromise();

  ExecutionContext* context = ExecutionContext::From(script_state);
  Document* document = ToDocumentOrNull(context);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  if (document && (!document->domWindow() || !document->GetFrame())) {
    resolver->Reject(DOMException::Create(
          kInvalidStateError, "Document is detached from window."));
    return promise;
  }

  // When the "applicationServerKey" property hasn't been explicitly passed to
  // the PushManager.subscribe() call, we'll attempt to get it from the manifest
  // if the call has been made from a Document context.
  //
  // Workers do not have an associated manifest, so we'll skip the check there.
  if (document && web_options.application_server_key.IsEmpty()) {
    PushMessagingBridge::From(registration_)
        ->GetApplicationServerKeyFromManifest(WTF::Bind(
            &PushManager::SubscribeWithApplicationServerKey,
            WrapWeakPersistent(this), WrapPersistent(resolver), web_options));
  } else {
    SubscribeWithApplicationServerKey(resolver, web_options,
                                      web_options.application_server_key);
  }

  return promise;
}

void PushManager::SubscribeWithApplicationServerKey(
    ScriptPromiseResolver* resolver,
    const WebPushSubscriptionOptions& options,
    const String& application_server_key) {
  if (application_server_key.IsEmpty()) {
    resolver->Reject(
        DOMException::Create(kAbortError, kMissingApplicationServerKeyMessage));
    return;
  }

  WebPushSubscriptionOptions new_options;
  new_options.user_visible_only = options.user_visible_only;
  new_options.application_server_key = application_server_key;

  PushMessagingBridge::From(registration_)
      ->RequestPermission(WTF::Bind(
          &PushManager::SubscribeWithPermissionRequested,
          WrapWeakPersistent(this), WrapPersistent(resolver), new_options));
}

void PushManager::SubscribeWithPermissionRequested(
    ScriptPromiseResolver* resolver,
    const WebPushSubscriptionOptions& options,
    mojom::blink::PermissionStatus permission_status) {
  if (permission_status != mojom::blink::PermissionStatus::GRANTED) {
    resolver->Reject(
        DOMException::Create(kNotAllowedError, kPermissionDeniedMessage));
    return;
  }

  PushProvider()->Subscribe(
      registration_->WebRegistration(), options,
      Frame::HasTransientUserActivation(nullptr, true /* checkIfMainThread */),
      std::make_unique<PushSubscriptionCallbacks>(resolver, registration_));
}

ScriptPromise PushManager::getSubscription(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  PushProvider()->GetSubscription(
      registration_->WebRegistration(),
      std::make_unique<PushSubscriptionCallbacks>(resolver, registration_));
  return promise;
}

ScriptPromise PushManager::permissionState(
    ScriptState* script_state,
    const PushSubscriptionOptionsInit& options,
    ExceptionState& exception_state) {
  if (ExecutionContext::From(script_state)->IsDocument()) {
    Document* document = ToDocument(ExecutionContext::From(script_state));
    if (!document->domWindow() || !document->GetFrame())
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kInvalidStateError,
                               "Document is detached from window."));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // The `userVisibleOnly` flag on |options| must be set, as it's intended to be
  // a contract with the developer that they will show a notification upon
  // receiving a push message. Permission is denied without this setting.
  //
  // TODO(peter): Would it be better to resolve DENIED rather than rejecting?
  if (!options.hasUserVisibleOnly() || !options.userVisibleOnly()) {
    resolver->Reject(
        DOMException::Create(kNotSupportedError, kUserVisibleOnlyRequired));
    return promise;
  }

  PushMessagingBridge::From(registration_)
      ->GetPermissionStatus(WTF::Bind(&PushManager::GotPermissionStatus,
                                      WrapWeakPersistent(this),
                                      WrapPersistent(resolver)));

  return promise;
}

void PushManager::GotPermissionStatus(ScriptPromiseResolver* resolver,
                                      mojom::blink::PermissionStatus status) {
  resolver->Resolve(PermissionStatusToString(status));
}

void PushManager::Trace(blink::Visitor* visitor) {
  visitor->Trace(registration_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
