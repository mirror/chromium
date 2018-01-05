// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/quota/QuotaDispatcher.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/common/quota/quota_types.mojom-blink.h"

namespace blink {

using mojom::blink::QuotaStatusCode;
using mojom::blink::StorageType;

// static
QuotaDispatcher* QuotaDispatcher::From(ExecutionContext* execution_context) {
  DCHECK(execution_context);
  DCHECK(execution_context->IsContextThread());

  QuotaDispatcher* dispatcher = static_cast<QuotaDispatcher*>(
      Supplement<ExecutionContext>::From(execution_context, SupplementName()));
  if (!dispatcher) {
    dispatcher = new QuotaDispatcher(*execution_context);
    Supplement<ExecutionContext>::ProvideTo(*execution_context,
                                            SupplementName(), dispatcher);
  }

  return dispatcher;
}

// static
const char* QuotaDispatcher::SupplementName() {
  return "QuotaDispatcher";
}

QuotaDispatcher::QuotaDispatcher(ExecutionContext& execution_context)
    : Supplement<ExecutionContext>(execution_context) {}

QuotaDispatcher::~QuotaDispatcher() {}

void QuotaDispatcher::QueryStorageUsageAndQuota(
    service_manager::InterfaceProvider* interface_provider,
    const scoped_refptr<const SecurityOrigin> origin,
    StorageType type,
    mojom::blink::QuotaDispatcherHost::QueryStorageUsageAndQuotaCallback
        callback) {
  DCHECK(callback);
  QuotaHost(interface_provider)
      ->QueryStorageUsageAndQuota(
          origin, type,
          mojo::WrapCallbackWithDefaultInvokeIfNotRun(
              std::move(callback), mojom::QuotaStatusCode::kErrorAbort, 0, 0));
}

void QuotaDispatcher::RequestStorageQuota(
    service_manager::InterfaceProvider* interface_provider,
    const scoped_refptr<const SecurityOrigin> origin,
    StorageType type,
    int64_t requested_size,
    mojom::blink::QuotaDispatcherHost::RequestStorageQuotaCallback callback) {
  DCHECK(callback);
  QuotaHost(interface_provider)
      ->RequestStorageQuota(
          origin, type, requested_size,
          mojo::WrapCallbackWithDefaultInvokeIfNotRun(
              std::move(callback), mojom::QuotaStatusCode::kErrorAbort, 0, 0));
}

void QuotaDispatcher::Trace(Visitor* visitor) {
  Supplement<ExecutionContext>::Trace(visitor);
}

mojom::blink::QuotaDispatcherHost* QuotaDispatcher::QuotaHost(
    service_manager::InterfaceProvider* interface_provider) {
  if (!quota_host_)
    interface_provider->GetInterface(mojo::MakeRequest(&quota_host_));
  return quota_host_.get();
}

}  // namespace blink
