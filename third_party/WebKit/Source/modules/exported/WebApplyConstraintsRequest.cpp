// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/web/WebApplyConstraintsRequest.h"

#include "modules/mediastream/ApplyConstraintsRequest.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebMediaConstraints.h"
#include "public/platform/WebMediaStreamTrack.h"
#include "public/platform/WebString.h"

namespace blink {

WebApplyConstraintsRequest::WebApplyConstraintsRequest(
    ApplyConstraintsRequest* request)
    : private_(request) {}

WebApplyConstraintsRequest::~WebApplyConstraintsRequest() {
  private_.Reset();
}

WebMediaStreamTrack WebApplyConstraintsRequest::Track() const {
  DCHECK(!IsNull());
  return private_->Track();
}

WebMediaConstraints WebApplyConstraintsRequest::Constraints() const {
  DCHECK(!IsNull());
  return private_->Constraints();
}

void WebApplyConstraintsRequest::RequestSucceeded() {
  DCHECK(!IsNull());
  private_->RequestSucceeded();
}

void WebApplyConstraintsRequest::RequestFailed(const WebString& constraint,
                                               const WebString& message) {
  DCHECK(!IsNull());
  private_->RequestFailed(constraint, message);
}

void WebApplyConstraintsRequest::RequestNotSupported(const WebString& message) {
  DCHECK(!IsNull());
  private_->RequestNotSupported(message);
}

bool WebApplyConstraintsRequest::operator==(
    const WebApplyConstraintsRequest& other) const {
  return private_.Get() == other.private_.Get();
}

void WebApplyConstraintsRequest::Assign(
    const WebApplyConstraintsRequest& other) {
  private_ = other.private_;
}

}  // namespace blink
