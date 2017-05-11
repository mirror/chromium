// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSING_DATA_CORE_COUNTERS_BOOKMARK_MODEL_HELPER_H_
#define COMPONENTS_BROWSING_DATA_CORE_COUNTERS_BOOKMARK_MODEL_HELPER_H_

#include "base/callback.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"

namespace browsing_data {

using BookmarkModelCallback =
    base::OnceCallback<void(bookmarks::BookmarkModel*)>;

// This class waits for the |bookmark_model| to load and executes |callback|
// afterwards.
class BookmarkModelHelper : public bookmarks::BaseBookmarkModelObserver {
 public:
  BookmarkModelHelper(bookmarks::BookmarkModel* bookmark_model,
                      BookmarkModelCallback callback);
  ~BookmarkModelHelper() override;

  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;

  void BookmarkModelChanged() override {}

 private:
  bookmarks::BookmarkModel* bookmark_model_;
  BookmarkModelCallback callback_;
};

}  // namespace browsing_data

#endif  // COMPONENTS_BROWSING_DATA_CORE_COUNTERS_BOOKMARK_MODEL_HELPER_H_
