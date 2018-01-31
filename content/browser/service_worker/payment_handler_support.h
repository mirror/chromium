// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_PAYMENT_HANDLER_SUPPORT_H_
#define CONTENT_BROWSER_SERVICE_WORKER_PAYMENT_HANDLER_SUPPORT_H_

#include "base/callback.h"
#include "third_party/WebKit/common/service_worker/service_worker.mojom.h"

class GURL;

namespace content {

class ServiceWorkerContextCore;

class PaymentHandlerSupport {
 public:
  using OpenWindowFallback = base::OnceCallback<void(
      blink::mojom::ServiceWorkerHost::
          OpenPaymentHandlerWindowCallback /* response_callback */)>;
  using ShowPaymentHandlerWindowCallback = base::OnceCallback<void(
      blink::mojom::ServiceWorkerHost::
          OpenPaymentHandlerWindowCallback /* response_callback */,
      bool /* success */,
      int /* render_process_id */,
      int /* render_frame_id */)>;

  // Requests to open a Payment Handler window. The embedder of content may
  // support or not support this operation, if not |fallback| is invoked to open
  // a normal window, and for either way |response_callback| will definitely be
  // invoked to return the result.
  static void ShowPaymentHandlerWindow(
      const GURL& url,
      ServiceWorkerContextCore* context,
      ShowPaymentHandlerWindowCallback callback,
      OpenWindowFallback fallback,
      blink::mojom::ServiceWorkerHost::OpenPaymentHandlerWindowCallback
          response_callback);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_PAYMENT_HANDLER_SUPPORT_H_
