// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PaymentHandlerUtils_h
#define PaymentHandlerUtils_h

#include "public/platform/modules/serviceworker/WebServiceWorkerResponseError.h"

namespace blink {

class ExecutionContext;

void ReportResponseError(ExecutionContext*, WebServiceWorkerResponseError);

}  // namespace blink

#endif
