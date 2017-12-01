// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RTCVoidRequestPromiseImpl_h
#define RTCVoidRequestPromiseImpl_h

#include "platform/peerconnection/RTCVoidRequest.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ScriptPromiseResolver;

// The requester of an RTCVoidRequestPromiseImpl is referenced by the request
// until resolved or rejected. The requester can cancel a promise that is about
// to be resolved or rejected, detaching the promise (releasing internal
// resources) and leaving it in a pending state.
class RTCPromiseRequester : public GarbageCollectedMixin {
 public:
  virtual ~RTCPromiseRequester() {}

  virtual bool DetachOnResolveOrReject() const = 0;

  virtual void Trace(blink::Visitor* visitor) {}
};

class RTCVoidRequestPromiseImpl final : public RTCVoidRequest {
 public:
  static RTCVoidRequestPromiseImpl* Create(ScriptPromiseResolver*,
                                           RTCPromiseRequester* = nullptr);
  ~RTCVoidRequestPromiseImpl() override;

  // RTCVoidRequest
  void RequestSucceeded() override;
  void RequestFailed(const String& error) override;

  virtual void Trace(blink::Visitor*);

 private:
  RTCVoidRequestPromiseImpl(ScriptPromiseResolver*, RTCPromiseRequester*);

  void RequestCompleted();

  bool is_pending_;
  Member<ScriptPromiseResolver> resolver_;
  Member<RTCPromiseRequester> requester_;
};

}  // namespace blink

#endif  // RTCVoidRequestPromiseImpl_h
