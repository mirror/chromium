// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_QUOTA_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_QUOTA_DISPATCHER_HOST_H_

#include "base/containers/id_map.h"
#include "base/macros.h"
#include "content/common/quota_messages.mojom.h"
#include "content/public/browser/browser_message_filter.h"

class GURL;

namespace storage {
class QuotaManager;
}

namespace content {
class QuotaPermissionContext;

class QuotaDispatcherHost : public content::mojom::QuotaMessages {
 public:
  static void Create(int process_id,
                     storage::QuotaManager* quota_manager,
                     QuotaPermissionContext* permission_context,
                     mojom::QuotaMessagesRequest request);

  QuotaDispatcherHost(int process_id,
                      storage::QuotaManager* quota_manager,
                      QuotaPermissionContext* permission_context);

  ~QuotaDispatcherHost() override;

  // content::mojom::QuotaMessages:
  void RequestStorageQuota(int64_t request_id,
                           int64_t render_frame_id,
                           const GURL& origin_url,
                           storage::StorageType storage_type,
                           uint64_t requested_size,
                           RequestStorageQuotaCallback callback) override;
  void QueryStorageUsageAndQuota(
      int64_t request_id,
      const GURL& origin_url,
      storage::StorageType storage_type,
      QueryStorageUsageAndQuotaCallback callback) override;

 private:
  class RequestDispatcher;
  class QueryUsageAndQuotaDispatcher;
  class RequestQuotaDispatcher;

  // The ID of this process.
  int process_id_;

  storage::QuotaManager* quota_manager_;
  scoped_refptr<QuotaPermissionContext> permission_context_;

  base::IDMap<std::unique_ptr<RequestDispatcher>> outstanding_requests_;

  base::WeakPtrFactory<QuotaDispatcherHost> weak_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(QuotaDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_QUOTA_DISPATCHER_HOST_H_
