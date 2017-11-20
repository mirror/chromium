// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/quota_dispatcher.h"

#include <memory>
#include <utility>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_local.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/render_thread.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/WebKit/public/platform/WebStorageQuotaCallbacks.h"
#include "third_party/WebKit/public/platform/WebStorageQuotaType.h"
#include "url/gurl.h"

using blink::WebStorageQuotaCallbacks;
using blink::WebStorageQuotaError;
using blink::WebStorageQuotaType;
using storage::QuotaStatusCode;
using storage::StorageType;

namespace content {

static base::LazyInstance<base::ThreadLocalPointer<QuotaDispatcher>>::Leaky
    g_quota_dispatcher_tls = LAZY_INSTANCE_INITIALIZER;

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

QuotaDispatcher::QuotaDispatcher() {
  g_quota_dispatcher_tls.Pointer()->Set(this);

  RenderThread::Get()->GetConnector()->BindInterface(mojom::kBrowserServiceName,
                                                     &quota_messages_);
  quota_messages_.set_connection_error_handler(base::BindOnce(
      [] { LOG(ERROR) << "QuotaDispatcher connection failed."; }));
}

QuotaDispatcher::~QuotaDispatcher() {
  base::IDMap<std::unique_ptr<Callback>>::iterator iter(
      &pending_quota_callbacks_);
  while (!iter.IsAtEnd()) {
    iter.GetCurrentValue()->DidFail(storage::kQuotaErrorAbort);
    iter.Advance();
  }

  g_quota_dispatcher_tls.Pointer()->Set(nullptr);
}

QuotaDispatcher* QuotaDispatcher::ThreadSpecificInstance() {
  if (g_quota_dispatcher_tls.Pointer()->Get())
    return g_quota_dispatcher_tls.Pointer()->Get();

  QuotaDispatcher* dispatcher = new QuotaDispatcher();
  if (WorkerThread::GetCurrentId())
    WorkerThread::AddObserver(dispatcher);
  return dispatcher;
}

void QuotaDispatcher::WillStopCurrentWorkerThread() {
  delete this;
}

void QuotaDispatcher::QueryStorageUsageAndQuota(
    const GURL& origin_url,
    StorageType type,
    std::unique_ptr<Callback> callback) {
  DCHECK(callback);
  int request_id = CurrentWorkerId();
  pending_quota_callbacks_.AddWithID(std::move(callback), request_id);
  quota_messages_->QueryStorageUsageAndQuota(
      request_id, origin_url, type,
      base::BindOnce(&QuotaDispatcher::DidQueryStorageUsageAndQuota,
                     base::Unretained(this)));
}

void QuotaDispatcher::RequestStorageQuota(int render_frame_id,
                                          const GURL& origin_url,
                                          StorageType type,
                                          int64_t requested_size,
                                          std::unique_ptr<Callback> callback) {
  DCHECK(callback);
  DCHECK(CurrentWorkerId() == 0);
  int request_id = CurrentWorkerId();
  pending_quota_callbacks_.AddWithID(std::move(callback), request_id);

  quota_messages_->RequestStorageQuota(
      request_id, render_frame_id, origin_url, type, requested_size,
      base::BindOnce(&QuotaDispatcher::DidGrantStorageQuota,
                     base::Unretained(this)));
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
