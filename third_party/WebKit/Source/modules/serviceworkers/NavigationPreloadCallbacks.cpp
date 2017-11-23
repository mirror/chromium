// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/serviceworkers/NavigationPreloadCallbacks.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/serviceworkers/NavigationPreloadState.h"
#include "modules/serviceworkers/ServiceWorkerError.h"
#include "public/platform/WebString.h"
#include "public/platform/modules/serviceworker/navigation_preload_state.mojom-blink.h"

namespace blink {

EnableNavigationPreloadCallbacks::EnableNavigationPreloadCallbacks(
    ScriptPromiseResolver* resolver)
    : resolver_(resolver) {
  DCHECK(resolver_);
}

EnableNavigationPreloadCallbacks::~EnableNavigationPreloadCallbacks() {}

void EnableNavigationPreloadCallbacks::OnSuccess() {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Resolve();
}

void EnableNavigationPreloadCallbacks::OnError(
    const WebServiceWorkerError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Reject(ServiceWorkerError::Take(resolver_.Get(), error));
}

GetNavigationPreloadStateCallbacks::GetNavigationPreloadStateCallbacks(
    ScriptPromiseResolver* resolver)
    : resolver_(resolver) {
  DCHECK(resolver_);
}

GetNavigationPreloadStateCallbacks::~GetNavigationPreloadStateCallbacks() {}

void GetNavigationPreloadStateCallbacks::OnSuccess(
    const mojom::blink::NavigationPreloadState& state) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  NavigationPreloadState dict;
  dict.setEnabled(state.enabled);
  dict.setHeaderValue(blink::WebString::FromUTF8(state.header));
  resolver_->Resolve(dict);
}

void GetNavigationPreloadStateCallbacks::OnError(
    const WebServiceWorkerError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Reject(ServiceWorkerError::Take(resolver_.Get(), error));
}

SetNavigationPreloadHeaderCallbacks::SetNavigationPreloadHeaderCallbacks(
    ScriptPromiseResolver* resolver)
    : resolver_(resolver) {
  DCHECK(resolver_);
}

SetNavigationPreloadHeaderCallbacks::~SetNavigationPreloadHeaderCallbacks() {}

void SetNavigationPreloadHeaderCallbacks::OnSuccess() {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Resolve();
}

void SetNavigationPreloadHeaderCallbacks::OnError(
    const WebServiceWorkerError& error) {
  if (!resolver_->GetExecutionContext() ||
      resolver_->GetExecutionContext()->IsContextDestroyed())
    return;
  resolver_->Reject(ServiceWorkerError::Take(resolver_.Get(), error));
}

}  // namespace blink
