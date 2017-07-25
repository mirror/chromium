// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_TEST_UTIL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_TEST_UTIL_H_

#include <memory>
#include <set>

#include "base/callback_forward.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "testing/gtest/include/gtest/gtest.h"

class GURL;

namespace base {
class ScopedTempDir;
}  // namespace base

namespace offline_pages {
struct PrefetchItem;

const int kStoreCommandFailed = -1;

class PrefetchStoreTestUtil {
 public:
  PrefetchStoreTestUtil();
  ~PrefetchStoreTestUtil();

  void BuildStore();
  void BuildStoreInMemory();

  void CountPrefetchItems(PrefetchStore::ResultCallback<int> result_callback);

  void GetAllItems(std::set<PrefetchItem>* all_items,
                   PrefetchStore::ResultCallback<std::size_t> result_callback);

  void ZombifyPrefetchItem(const std::string& name_space,
                           const GURL& url,
                           PrefetchStore::ResultCallback<int> reuslt_callback);

  void InsertItem(const PrefetchItem& item,
                  PrefetchStore::ResultCallback<bool> result_callback);

  // Releases the ownership of currently controlled store.
  std::unique_ptr<PrefetchStore> ReleaseStore();

  void DeleteStore();

  // Helper methods to create expect-true testing callbacks.
  static void ExpectTrue(bool value) { EXPECT_TRUE(value); }
  static PrefetchStore::ResultCallback<bool> ExpectTrueCallback() {
    return base::BindOnce(&ExpectTrue);
  }

  // Helper methods to create expect-eq testing callbacks.
  template <typename T>
  static void ExpectEq(T expected_count, T actual_count) {
    EXPECT_EQ(expected_count, actual_count);
  }
  template <typename T>
  static PrefetchStore::ResultCallback<T> ExpectEqCallback(T value) {
    return base::BindOnce(&ExpectEq<T>, value);
  }

  PrefetchStore* store() { return store_.get(); }

 private:
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<PrefetchStore> store_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchStoreTestUtil);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_TEST_UTIL_H_
