// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_CACHE_STORAGE_CONTEXT_H_
#define CONTENT_PUBLIC_BROWSER_CACHE_STORAGE_CONTEXT_H_

#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/cache_storage_usage_info.h"
#include "url/gurl.h"

namespace content {

// Represents the per-BrowserContext Cache Storage data.
class CacheStorageContext
    : public base::RefCountedThreadSafe<CacheStorageContext> {
 public:
  using GetUsageInfoCallback = base::Callback<void(
      const std::vector<CacheStorageUsageInfo>& usage_info)>;

  enum ObservationType { CACHE_LIST_CHANGED, CACHE_DATA_CHANGED };

  // Methods used in response to browsing data and quota manager requests.
  // Must be called on the IO thread.
  virtual void GetAllOriginsInfo(const GetUsageInfoCallback& callback) = 0;
  virtual void DeleteForOrigin(const GURL& origin_url) = 0;

  // Methods used for live updating in devtools.
  // Must be called on the IO thread.
  virtual void AddObserver(
      const GURL& origin,
      int64_t id,
      base::RepeatingCallback<void(ObservationType, const GURL&)> callback) = 0;
  virtual void RemoveObserver(const GURL& origin, int64_t id) = 0;

 protected:
  friend class base::RefCountedThreadSafe<CacheStorageContext>;
  virtual ~CacheStorageContext() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_CACHE_STORAGE_CONTEXT_H_
