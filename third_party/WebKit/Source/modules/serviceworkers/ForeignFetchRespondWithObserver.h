// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ForeignFetchRespondWithObserver_h
#define ForeignFetchRespondWithObserver_h

#include "modules/serviceworkers/FetchRespondWithObserver.h"
#include "services/network/public/interfaces/fetch_api.mojom-blink.h"

namespace blink {

// This class observes the service worker's handling of a ForeignFetchEvent and
// notifies the client.
class MODULES_EXPORT ForeignFetchRespondWithObserver final
    : public FetchRespondWithObserver {
 public:
  static ForeignFetchRespondWithObserver* Create(
      ExecutionContext*,
      int event_id,
      const KURL& request_url,
      network::mojom::FetchRequestMode,
      WebURLRequest::FetchRedirectMode,
      WebURLRequest::FrameType,
      WebURLRequest::RequestContext,
      scoped_refptr<const SecurityOrigin>,
      WaitUntilObserver*);

  void OnResponseFulfilled(const ScriptValue&) override;

 private:
  ForeignFetchRespondWithObserver(ExecutionContext*,
                                  int event_id,
                                  const KURL& request_url,
                                  network::mojom::FetchRequestMode,
                                  WebURLRequest::FetchRedirectMode,
                                  WebURLRequest::FrameType,
                                  WebURLRequest::RequestContext,
                                  scoped_refptr<const SecurityOrigin>,
                                  WaitUntilObserver*);

  scoped_refptr<const SecurityOrigin> request_origin_;
};

}  // namespace blink

#endif  // ForeignFetchRespondWithObserver_h
