// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanMakePaymentRespondWithObserver_h
#define CanMakePaymentRespondWithObserver_h

#include "modules/ModulesExport.h"
#include "modules/serviceworkers/RespondWithObserver.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerResponseError.h"

namespace blink {

class ExecutionContext;
class ScriptValue;
class WaitUntilObserver;

// This class observes the service worker's handling of a CanMakePaymentEvent
// and notifies the client.
class MODULES_EXPORT CanMakePaymentRespondWithObserver final
    : public RespondWithObserver {
 public:
  ~CanMakePaymentRespondWithObserver() override = default;

  static CanMakePaymentRespondWithObserver* Create(ExecutionContext*,
                                                   int event_id,
                                                   WaitUntilObserver*);

  void OnResponseRejected(WebServiceWorkerResponseError) override;
  void OnResponseFulfilled(const ScriptValue&) override;
  void OnNoResponse() override;

  DECLARE_VIRTUAL_TRACE();

 private:
  CanMakePaymentRespondWithObserver(ExecutionContext*,
                                    int event_id,
                                    WaitUntilObserver*);
};

}  // namespace blink

#endif  // CanMakePaymentRespondWithObserver_h
