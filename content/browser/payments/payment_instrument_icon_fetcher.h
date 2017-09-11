// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PAYMENTS_PAYMENT_INSTRUMENT_ICON_FETCHER_H_
#define CONTENT_BROWSER_PAYMENTS_PAYMENT_INSTRUMENT_ICON_FETCHER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/public/common/manifest.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_app.mojom.h"

namespace content {

class PaymentInstrumentIconFetcher {
 public:
  using PaymentInstrumentIconFetcherCallback =
      base::OnceCallback<void(const std::string&)>;

  static void Start(
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
      const std::vector<Manifest::Icon>& icons,
      PaymentInstrumentIconFetcherCallback callback);

  DISALLOW_IMPLICIT_CONSTRUCTORS(PaymentInstrumentIconFetcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PAYMENTS_PAYMENT_INSTRUMENT_ICON_FETCHER_H_
