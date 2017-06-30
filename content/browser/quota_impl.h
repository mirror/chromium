// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_QUOTA_IMPL_H_
#define CONTENT_BROWSER_QUOTA_IMPL_H_

#include "base/callback.h"
#include "base/id_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "storage/public/interfaces/quota.mojom.h"
#include "url/origin.h"

namespace content {

// Implements the Quota Mojo interface.
// TODO This service can be created from a xxxRenderFrameHost or a
// xxxRenderProcessHost.
// TODO It is owned by a xxxContext.
class CONTENT_EXPORT QuotaImpl : public quota::mojom::Quota {
 public:
  QuotaImpl();
  ~QuotaImpl() override;

 private:
  // TODO friend class QuotaImplTest;

  using RequestStorageQuotaCallback = base::OnceCallback<
      void(mojom::quota::QuotaStatusCode, int64_t, int64_t)>;
  using QueryStorageUsageAndQuotaCallback = base::OnceCallback<
      void(mojom::quota::QuotaStatusCode, int64_t, int64_t)>;

  // mojom::Quota.
  void RequestStorageQuota(
      quota::mojom::StorageType type,
      const RefPtr<::blink::SecurityOrigin>& origin,
      int64_t requested_size,
      bool user_gesture,
      quota::mojom::RequestStorageQuotaCallback callback) override;

  void QueryStorageUsageAndQuota(
      quota::mojom::StorageType type,
      const RefPtr<::blink::SecurityOrigin>& origin,
      quota::mojom::QueryStorageUsageAndQuotaCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(QuotaImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_QUOTA_IMPL_H_
