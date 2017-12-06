// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/quota_dispatcher.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/render_thread.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/platform/WebStorageQuotaCallbacks.h"
#include "third_party/WebKit/public/platform/WebStorageQuotaType.h"
#include "url/origin.h"

using blink::WebStorageQuotaCallbacks;
using blink::WebStorageQuotaError;
using blink::WebStorageQuotaType;
using storage::QuotaStatusCode;
using storage::StorageType;

namespace content {

namespace {

// QuotaDispatcher::Callback implementation for WebStorageQuotaCallbacks.
class WebStorageQuotaDispatcherCallback : public QuotaDispatcher::Callback {
 public:
  explicit WebStorageQuotaDispatcherCallback(
      blink::WebStorageQuotaCallbacks callback)
      : callbacks_(callback) {}
  ~WebStorageQuotaDispatcherCallback() override {}

  void DidQueryStorageUsageAndQuota(int64_t usage, int64_t quota) override {
    callbacks_.DidQueryStorageUsageAndQuota(usage, quota);
  }
  void DidGrantStorageQuota(int64_t usage, int64_t granted_quota) override {
    callbacks_.DidGrantStorageQuota(usage, granted_quota);
  }
  void DidFail(storage::QuotaStatusCode error) override {
    callbacks_.DidFail(static_cast<WebStorageQuotaError>(error));
  }

 private:
  blink::WebStorageQuotaCallbacks callbacks_;

  DISALLOW_COPY_AND_ASSIGN(WebStorageQuotaDispatcherCallback);
};

int CurrentWorkerId() {
  return WorkerThread::GetCurrentId();
}

}  // namespace

// static
QuotaDispatcher* QuotaDispatcher::From(
    blink::ExecutionContext* execution_context) {
  DCHECK(execution_context);
  DCHECK(execution_context->IsContextThread());

  QuotaDispatcher* dispatcher = static_cast<QuotaDispatcher*>(
      blink::Supplement<blink::ExecutionContext>::From(execution_context,
                                                       SupplementName()));
  if (!dispatcher) {
    dispatcher = new QuotaDispatcher(*execution_context);
    blink::Supplement<blink::ExecutionContext>::ProvideTo(
        *execution_context, SupplementName(), dispatcher);
    if (WorkerThread::GetCurrentId())
      WorkerThread::AddObserver(dispatcher);
  }

  return dispatcher;
}

// static
const char* QuotaDispatcher::SupplementName() {
  return "QuotaDispatcher";
}

QuotaDispatcher::QuotaDispatcher(blink::ExecutionContext& execution_context)
    : blink::Supplement<blink::ExecutionContext>(execution_context) {}

QuotaDispatcher::~QuotaDispatcher() {
  base::IDMap<std::unique_ptr<Callback>>::iterator iter(
      &pending_quota_callbacks_);
  while (!iter.IsAtEnd()) {
    iter.GetCurrentValue()->DidFail(storage::kQuotaErrorAbort);
    iter.Advance();
  }
}

void QuotaDispatcher::QueryStorageUsageAndQuota(
    service_manager::InterfaceProvider* interface_provider,
    const url::Origin& origin,
    StorageType type,
    std::unique_ptr<Callback> callback) {
  DCHECK(callback);
  int request_id = pending_quota_callbacks_.Add(std::move(callback));
  QuotaHost(interface_provider)
      ->QueryStorageUsageAndQuota(
          origin, type,
          base::BindOnce(&QuotaDispatcher::DidQueryStorageUsageAndQuota,
                         base::Unretained(this), request_id));
}

void QuotaDispatcher::RequestStorageQuota(
    service_manager::InterfaceProvider* interface_provider,
    int render_frame_id,
    const url::Origin& origin,
    StorageType type,
    int64_t requested_size,
    std::unique_ptr<Callback> callback) {
  DCHECK(callback);
  DCHECK_EQ(CurrentWorkerId(), 0)
      << "Requests may show permission UI and are not allowed from workers.";
  int request_id = pending_quota_callbacks_.Add(std::move(callback));
  QuotaHost(interface_provider)
      ->RequestStorageQuota(
          render_frame_id, origin, type, requested_size,
          base::BindOnce(&QuotaDispatcher::DidGrantStorageQuota,
                         base::Unretained(this), request_id));
}

// static
std::unique_ptr<QuotaDispatcher::Callback>
QuotaDispatcher::CreateWebStorageQuotaCallbacksWrapper(
    blink::WebStorageQuotaCallbacks callbacks) {
  return std::make_unique<WebStorageQuotaDispatcherCallback>(callbacks);
}

void QuotaDispatcher::DidGrantStorageQuota(int64_t request_id,
                                           storage::QuotaStatusCode status,
                                           int64_t current_usage,
                                           int64_t granted_quota) {
  if (status != storage::kQuotaStatusOk) {
    DidFail(request_id, status);
    return;
  }

  Callback* callback = pending_quota_callbacks_.Lookup(request_id);
  DCHECK(callback);
  callback->DidGrantStorageQuota(current_usage, granted_quota);
  pending_quota_callbacks_.Remove(request_id);
}

void QuotaDispatcher::DidQueryStorageUsageAndQuota(
    int64_t request_id,
    storage::QuotaStatusCode status,
    int64_t current_usage,
    int64_t current_quota) {
  if (status != storage::kQuotaStatusOk) {
    DidFail(request_id, status);
    return;
  }

  Callback* callback = pending_quota_callbacks_.Lookup(request_id);
  DCHECK(callback);
  callback->DidQueryStorageUsageAndQuota(current_usage, current_quota);
  pending_quota_callbacks_.Remove(request_id);
}

void QuotaDispatcher::DidFail(
    int request_id,
    QuotaStatusCode error) {
  Callback* callback = pending_quota_callbacks_.Lookup(request_id);
  DCHECK(callback);
  callback->DidFail(error);
  pending_quota_callbacks_.Remove(request_id);
}

mojom::QuotaDispatcherHost* QuotaDispatcher::QuotaHost(
    service_manager::InterfaceProvider* interface_provider) {
  if (!quota_host_)
    interface_provider->GetInterface(mojo::MakeRequest(&quota_host_));
  return quota_host_.get();
}

static_assert(int(blink::kWebStorageQuotaTypeTemporary) ==
                  int(storage::kStorageTypeTemporary),
              "mismatching enums: kStorageTypeTemporary");
static_assert(int(blink::kWebStorageQuotaTypePersistent) ==
                  int(storage::kStorageTypePersistent),
              "mismatching enums: kStorageTypePersistent");

static_assert(int(blink::kWebStorageQuotaErrorNotSupported) ==
                  int(storage::kQuotaErrorNotSupported),
              "mismatching enums: kQuotaErrorNotSupported");
static_assert(int(blink::kWebStorageQuotaErrorAbort) ==
                  int(storage::kQuotaErrorAbort),
              "mismatching enums: kQuotaErrorAbort");

}  // namespace content
