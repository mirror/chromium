// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browsing_data/core/counters/bookmark_counter.h"

#include "base/memory/ptr_util.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/browsing_data/core/counters/bookmark_model_helper.h"

namespace {

int CountBookmarksFromNode(const bookmarks::BookmarkNode* node) {
  int count = 0;
  if (node->is_url()) {
    ++count;
  } else {
    for (int i = 0; i < node->child_count(); ++i)
      count += CountBookmarksFromNode(node->GetChild(i));
  }
  return count;
}

}  // namespace

namespace browsing_data {

const char BookmarkCounter::kFakePrefName[] =
    "browsing.data.fake.pref.name.password";

BookmarkCounter::BookmarkCounter(bookmarks::BookmarkModel* bookmark_model)
    : bookmark_model_(bookmark_model) {
  DCHECK(bookmark_model);
}

BookmarkCounter::~BookmarkCounter() {}

void BookmarkCounter::OnInitialized() {}

const char* BookmarkCounter::GetPrefName() const {
  // TODO(dullweber): Is there a better solution for counters without
  // preferences?
  return kFakePrefName;
}

void BookmarkCounter::Count() {
  if (bookmark_model_->loaded()) {
    CountBookmarks(bookmark_model_);
  } else {
    bookmark_model_helper_.reset(new BookmarkModelHelper(
        bookmark_model_, base::BindOnce(&BookmarkCounter::CountBookmarks,
                                        base::Unretained(this))));
  }
}

void BookmarkCounter::CountBookmarks(bookmarks::BookmarkModel* bookmark_model) {
  int count = CountBookmarksFromNode(bookmark_model->bookmark_bar_node()) +
              CountBookmarksFromNode(bookmark_model->other_node()) +
              CountBookmarksFromNode(bookmark_model->mobile_node());
  ReportResult(count);
}

}  // namespace browsing_data
