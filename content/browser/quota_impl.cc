// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/quota_impl.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_features.h"

using quota::mojom::blink::StorageType;
using quota::mojom::blink::RequestStorageQuotaCallback;
using quota::mojom::blink::QueryStorageUsageAndQuotaCallback;

namespace content {

QuotaImpl::QuotaImpl() {
}

QuotaImpl::~QuotaImpl() {
}

void QuotaImpl::RequestStorageQuota(
    StorageType type,
    const RefPtr<::blink::SecurityOrigin>& origin,
    int64_t requested_size,
    bool user_gesture,
    RequestStorageQuotaCallback callback) {
    // TODO
}

void QuotaImpl::QueryStorageUsageAndQuota(
    StorageType type,
    const RefPtr<::blink::SecurityOrigin>& origin,
    QueryStorageUsageAndQuotaCallback callback) {
    // TODO
}

}  // namespace content
