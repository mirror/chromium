// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/mediastream/ApplyConstraintsRequest.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "modules/mediastream/MediaStreamTrack.h"
#include "modules/mediastream/MediaTrackConstraints.h"
#include "modules/mediastream/OverconstrainedError.h"
#include "platform/mediastream/MediaStreamComponent.h"
#include "public/platform/WebMediaConstraints.h"
#include "public/platform/WebMediaStreamTrack.h"

namespace blink {

ApplyConstraintsRequest* ApplyConstraintsRequest::Create(
    MediaStreamTrack* track,
    const WebMediaConstraints& constraints,
    ScriptPromiseResolver* resolver) {
  return new ApplyConstraintsRequest(track, constraints, resolver);
}

ApplyConstraintsRequest::ApplyConstraintsRequest(
    MediaStreamTrack* track,
    const WebMediaConstraints& constraints,
    ScriptPromiseResolver* resolver)
    : track_(track), constraints_(constraints), resolver_(resolver) {}

WebMediaStreamTrack ApplyConstraintsRequest::Track() const {
  return track_->Component();
}

WebMediaConstraints ApplyConstraintsRequest::Constraints() const {
  return constraints_;
}

void ApplyConstraintsRequest::RequestSucceeded() {
  resolver_->Resolve();
}

void ApplyConstraintsRequest::RequestFailed(const String& constraint,
                                            const String& message) {
  DCHECK(!constraint.IsEmpty());
  resolver_->Reject(OverconstrainedError::Create(constraint, message));
}

void ApplyConstraintsRequest::RequestNotSupported(const String& message) {
  resolver_->Reject(DOMException::Create(kNotSupportedError, message));
}

DEFINE_TRACE(ApplyConstraintsRequest) {
  visitor->Trace(track_);
  visitor->Trace(resolver_);
}

}  // namespace blink
