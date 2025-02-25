// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_LOCAL_STORAGE_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_LOCAL_STORAGE_HELPER_H_

#include <list>
#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"

// Mock for BrowsingDataLocalStorageHelper.
// Use AddLocalStorageSamples() or add directly to response_ list, then
// call Notify().
class MockBrowsingDataLocalStorageHelper
    : public BrowsingDataLocalStorageHelper {
 public:
  explicit MockBrowsingDataLocalStorageHelper(Profile* profile);

  // BrowsingDataLocalStorageHelper implementation.
  void StartFetching(const FetchCallback& callback) override;
  void DeleteOrigin(const GURL& origin, base::OnceClosure callback) override;

  // Adds some LocalStorageInfo samples.
  void AddLocalStorageSamples();
  void AddLocalStorageSamplesWithSuborigins();

  // Notifies the callback.
  void Notify();

  // Marks all local storage files as existing.
  void Reset();

  // Returns true if all local storage files were deleted since the last Reset()
  // invocation.
  bool AllDeleted();

  GURL last_deleted_origin_;

 private:
  ~MockBrowsingDataLocalStorageHelper() override;

  FetchCallback callback_;

  std::map<const GURL, bool> origins_;

  std::list<LocalStorageInfo> response_;

  DISALLOW_COPY_AND_ASSIGN(MockBrowsingDataLocalStorageHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_LOCAL_STORAGE_HELPER_H_
