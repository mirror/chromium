// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSING_DATA_CORE_COUNTERS_BOOKMARKS_COUNTER_H_
#define COMPONENTS_BROWSING_DATA_CORE_COUNTERS_BOOKMARKS_COUNTER_H_

#include <memory>

#include "components/browsing_data/core/counters/browsing_data_counter.h"

namespace {
class BookmarkModelHelper;
}

namespace bookmarks {
class BookmarkModel;
}

namespace browsing_data {

class BookmarkCounter : public browsing_data::BrowsingDataCounter {
 public:
  static const char kFakePrefName[];

  explicit BookmarkCounter(bookmarks::BookmarkModel* bookmark_model);
  ~BookmarkCounter() override;

  void OnInitialized() override;

  const char* GetPrefName() const override;

 private:
  void Count() override;
  void CountBookmarks(bookmarks::BookmarkModel* bookmark_model);

  bookmarks::BookmarkModel* bookmark_model_;
  std::unique_ptr<BookmarkModelHelper> bookmark_model_helper_;
};

}  // namespace browsing_data

#endif  // COMPONENTS_BROWSING_DATA_CORE_COUNTERS_BOOKMARKS_COUNTER_H_
