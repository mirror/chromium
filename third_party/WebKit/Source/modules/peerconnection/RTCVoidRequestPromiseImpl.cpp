// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/peerconnection/RTCVoidRequestPromiseImpl.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"

namespace blink {

RTCVoidRequestPromiseImpl* RTCVoidRequestPromiseImpl::Create(
    ScriptPromiseResolver* resolver,
    RTCPromiseRequester* requester) {
  return new RTCVoidRequestPromiseImpl(resolver, requester);
}

RTCVoidRequestPromiseImpl::RTCVoidRequestPromiseImpl(
    ScriptPromiseResolver* resolver,
    RTCPromiseRequester* requester)
    : is_pending_(true), resolver_(resolver), requester_(requester) {
  DCHECK(resolver_);
}

RTCVoidRequestPromiseImpl::~RTCVoidRequestPromiseImpl() {}

void RTCVoidRequestPromiseImpl::RequestSucceeded() {
  if (!is_pending_)
    return;

  if (!requester_ || !requester_->DetachOnResolveOrReject()) {
    resolver_->Resolve();
  } else {
    // This is needed to have the resolver release its internal resources
    // while leaving the associated promise pending as specified.
    resolver_->Detach();
  }

  RequestCompleted();
}

void RTCVoidRequestPromiseImpl::RequestFailed(const String& error) {
  if (!is_pending_)
    return;

  if (!requester_ || !requester_->DetachOnResolveOrReject()) {
    // TODO(guidou): The error code should come from the content layer. See
    // crbug.com/589455
    resolver_->Reject(DOMException::Create(kOperationError, error));
  } else {
    // This is needed to have the resolver release its internal resources
    // while leaving the associated promise pending as specified.
    resolver_->Detach();
  }

  RequestCompleted();
}

void RTCVoidRequestPromiseImpl::RequestCompleted() {
  is_pending_ = false;
  requester_.Clear();
}

void RTCVoidRequestPromiseImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(resolver_);
  visitor->Trace(requester_);
  RTCVoidRequest::Trace(visitor);
}

}  // namespace blink
