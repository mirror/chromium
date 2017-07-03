// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PaymentRespondWithObserverBase_h
#define PaymentRespondWithObserverBase_h

#include "modules/ModulesExport.h"
#include "modules/serviceworkers/RespondWithObserver.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerResponseError.h"

namespace blink {

class ExecutionContext;
class WaitUntilObserver;

// This class observes the service worker's handling of a PaymentRequestEvent
// and notifies the client.
class PaymentRespondWithObserverBase : public RespondWithObserver {
 public:
  ~PaymentRespondWithObserverBase() override = default;

  void OnResponseRejected(WebServiceWorkerResponseError) override;

 protected:
  PaymentRespondWithObserverBase(ExecutionContext*,
                                 int event_id,
                                 WaitUntilObserver*);
};

}  // namespace blink

#endif  // PaymentRespondWithObserverBase_h
