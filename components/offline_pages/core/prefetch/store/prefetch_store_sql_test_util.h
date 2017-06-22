// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_TEST_UTIL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_TEST_UTIL_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql.h"

class GURL;

namespace base {
class ScopedTempDir;
}  // namespace base

namespace offline_pages {

class PrefetchStoreSQLTestUtil {
 public:
  PrefetchStoreSQLTestUtil();
  ~PrefetchStoreSQLTestUtil();

  void BuildStore();
  void BuildStoreInMemory();

  void CountPrefetchItems(
      PrefetchStoreSQL::ResultCallback<int> result_callback);

  void ZombifyPrefetchItem(
      const std::string& name_space,
      const GURL& url,
      PrefetchStoreSQL::ResultCallback<int> reuslt_callback);

  // Releases the ownership of currently controlled store.
  std::unique_ptr<PrefetchStoreSQL> ReleaseStore();

  PrefetchStoreSQL* store() { return store_.get(); }

 private:
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<PrefetchStoreSQL> store_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchStoreSQLTestUtil);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_TEST_UTIL_H_
