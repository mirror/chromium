// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_STORAGE_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_STORAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "content/browser/devtools/protocol/storage.h"
#include "url/origin.h"

namespace content {

class RenderFrameHostImpl;

namespace protocol {

class StorageHandler : public DevToolsDomainHandler,
                       public Storage::Backend {
 public:
  StorageHandler();
  ~StorageHandler() override;

  void Wire(UberDispatcher* dispatcher) override;
  void SetRenderFrameHost(RenderFrameHostImpl* host) override;

  Response ClearDataForOrigin(
      const std::string& origin,
      const std::string& storage_types) override;
  void GetUsageAndQuota(
      const String& origin,
      std::unique_ptr<GetUsageAndQuotaCallback> callback) override;
  // Ignore all double calls to track an origin.
  void TrackOrigin(const String& origin,
                   std::unique_ptr<TrackOriginCallback> callback) override;
  void UntrackOrigin(const String& origin,
                     std::unique_ptr<UntrackOriginCallback> callback) override;

  void NotifyCacheStorageListChanged(const String& origin);

 private:
  std::unique_ptr<Storage::Frontend> frontend_;
  RenderFrameHostImpl* host_;
  // Will never have multiple of the same origin.
  std::vector<std::pair<url::Origin, int64_t>> observers_;

  base::WeakPtrFactory<StorageHandler> ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(StorageHandler);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_STORAGE_HANDLER_H_
