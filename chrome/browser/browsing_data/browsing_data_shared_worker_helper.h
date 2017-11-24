// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_SHARED_WORKER_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_SHARED_WORKER_HELPER_H_

#include <stddef.h>

#include <list>
#include <set>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "url/gurl.h"

namespace content {
class StoragePartition;
class ResourceContext;
}  // namespace content

// Shared Workers don't persist browsing data outside of the regular storage
// mechanisms of their origin, however, we need the helper anyways to be able
// to show them as cookie-like data.
class BrowsingDataSharedWorkerHelper
    : public base::RefCountedThreadSafe<BrowsingDataSharedWorkerHelper> {
 public:
  using FetchCallback = base::OnceCallback<void(
      const std::list<
          std::pair<GURL /* worker script URL */, std::string /* name */>>&)>;

  BrowsingDataSharedWorkerHelper(content::StoragePartition* storage_partition,
                                 content::ResourceContext* resource_context);

  // Starts the fetching process returning the list of shared workers, which
  // will notify its completion via |callback|. This must be called only in the
  // UI thread.
  virtual void StartFetching(FetchCallback callback);

  // Requests the given Shared Worker to be deleted.
  virtual void DeleteSharedWorker(const GURL& worker, const std::string& name);

 protected:
  virtual ~BrowsingDataSharedWorkerHelper();

 private:
  friend class base::RefCountedThreadSafe<BrowsingDataSharedWorkerHelper>;

  content::StoragePartition* storage_partition_;
  content::ResourceContext* resource_context_;

  DISALLOW_COPY_AND_ASSIGN(BrowsingDataSharedWorkerHelper);
};

// This class is an implementation of BrowsingDataSharedWorkerHelper that does
// not fetch its information from the Shared Worker context, but is passed the
// info as a parameter.
class CannedBrowsingDataSharedWorkerHelper
    : public BrowsingDataSharedWorkerHelper {
 public:
  // Contains information about a Shared Worker.
  struct PendingSharedWorkerUsageInfo {
    PendingSharedWorkerUsageInfo(const GURL& worker, const std::string& name);
    PendingSharedWorkerUsageInfo(const PendingSharedWorkerUsageInfo& other);
    ~PendingSharedWorkerUsageInfo();

    bool operator<(const PendingSharedWorkerUsageInfo& other) const;

    GURL worker;
    std::string name;
  };

  CannedBrowsingDataSharedWorkerHelper(
      content::StoragePartition* storage_partition,
      content::ResourceContext* resource_context);

  // Adds Shared Worker to the set of canned Shared Workers that is returned by
  // this helper.
  void AddSharedWorker(const GURL& worker, const std::string& name);

  // Clears the list of canned Shared Workers.
  void Reset();

  // True if no Shared Workers are currently in the set.
  bool empty() const;

  // Returns the number of Shared Workers in the set.
  size_t GetSharedWorkerCount() const;

  // Returns the current list of Shared Workers.
  const std::set<
      CannedBrowsingDataSharedWorkerHelper::PendingSharedWorkerUsageInfo>&
  GetSharedWorkerUsageInfo() const;

  // BrowsingDataSharedWorkerHelper methods.
  void StartFetching(FetchCallback callback) override;
  void DeleteSharedWorker(const GURL& worker, const std::string& name) override;

 private:
  ~CannedBrowsingDataSharedWorkerHelper() override;

  std::set<PendingSharedWorkerUsageInfo> pending_shared_worker_info_;

  DISALLOW_COPY_AND_ASSIGN(CannedBrowsingDataSharedWorkerHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_SHARED_WORKER_HELPER_H_
