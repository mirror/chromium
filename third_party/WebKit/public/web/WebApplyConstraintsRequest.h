// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebApplyConstraintsRequest_h
#define WebApplyConstraintsRequest_h

#include "public/platform/WebCommon.h"
#include "public/platform/WebPrivatePtr.h"

namespace blink {

class ApplyConstraintsRequest;
class WebMediaConstraints;
class WebMediaStreamTrack;
class WebString;

class BLINK_EXPORT WebApplyConstraintsRequest {
 public:
  explicit WebApplyConstraintsRequest(
      const WebApplyConstraintsRequest& request) {
    Assign(request);
  }
  ~WebApplyConstraintsRequest();

  WebApplyConstraintsRequest& operator=(
      const WebApplyConstraintsRequest& other) {
    Assign(other);
    return *this;
  }

  bool operator==(const WebApplyConstraintsRequest& other) const;

  bool IsNull() const { return private_.IsNull(); }

  WebMediaStreamTrack Track() const;
  WebMediaConstraints Constraints() const;

  void RequestSucceeded();
  void RequestFailed(const WebString& constraint, const WebString& message);

  // TODO(guidou): Remove this method. http://crbug.com/338503
  void RequestNotSupported(const WebString& message);

#if INSIDE_BLINK
  WebApplyConstraintsRequest() = default;
  explicit WebApplyConstraintsRequest(ApplyConstraintsRequest*);
#endif

 private:
  void Assign(const WebApplyConstraintsRequest&);

  WebPrivatePtr<ApplyConstraintsRequest> private_;
};

}  // namespace blink

#endif  // WebApplyConstraintsRequest_h
