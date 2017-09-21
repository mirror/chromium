// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/mediastream/ApplyConstraintsRequest.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/mediastream/OverconstrainedError.h"

namespace blink {

ApplyConstraintsRequest* ApplyConstraintsRequest::Create(
    const WebMediaStreamTrack& track,
    const WebMediaConstraints& constraints,
    ScriptPromiseResolver* resolver) {
  return new ApplyConstraintsRequest(track, constraints, resolver);
}

ApplyConstraintsRequest::ApplyConstraintsRequest(
    const WebMediaStreamTrack& track,
    const WebMediaConstraints& constraints,
    ScriptPromiseResolver* resolver)
    : track_(track), constraints_(constraints), resolver_(resolver) {}

WebMediaStreamTrack ApplyConstraintsRequest::Track() const {
  return track_;
}

WebMediaConstraints ApplyConstraintsRequest::Constraints() const {
  return constraints_;
}

void ApplyConstraintsRequest::RequestSucceeded() {
  resolver_->Resolve();
  track_.Reset();
}

void ApplyConstraintsRequest::RequestFailed(const String& constraint,
                                            const String& message) {
  DCHECK(!constraint.IsEmpty());
  resolver_->Reject(OverconstrainedError::Create(constraint, message));
  track_.Reset();
}

void ApplyConstraintsRequest::RequestNotSupported(const String& message) {
  resolver_->Reject(DOMException::Create(kNotSupportedError, message));
  track_.Reset();
}

DEFINE_TRACE(ApplyConstraintsRequest) {
  visitor->Trace(resolver_);
}

}  // namespace blink
