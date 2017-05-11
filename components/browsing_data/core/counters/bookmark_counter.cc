// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browsing_data/core/counters/bookmark_counter.h"

#include "base/memory/ptr_util.h"
#include "base/timer/timer.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"

using bookmarks::BookmarkModel;

namespace {

using BookmarkModelCallback = base::OnceCallback<void(BookmarkModel*)>;

class BookmarkModelHelper : public bookmarks::BaseBookmarkModelObserver {
 public:
  explicit BookmarkModelHelper(BookmarkModel* bookmark_model,
                               BookmarkModelCallback callback)
      : bookmark_model_(bookmark_model), callback_(std::move(callback)) {
    bookmark_model_->AddObserver(this);
  }

  ~BookmarkModelHelper() override { bookmark_model_->RemoveObserver(this); }

  void BookmarkModelLoaded(BookmarkModel* model, bool ids_reassigned) override {
    model->RemoveObserver(this);
    std::move(callback_).Run(model);
  };

  void BookmarkModelChanged() override {}

 private:
  BookmarkModel* bookmark_model_;
  BookmarkModelCallback callback_;
};

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

BookmarkCounter::BookmarkCounter(BookmarkModel* bookmark_model)
    : bookmark_model_(bookmark_model) {
  DCHECK(bookmark_model);
}

BookmarkCounter::~BookmarkCounter() {}

void BookmarkCounter::OnInitialized() {}

const char* BookmarkCounter::GetPrefName() const {
  // TODO(dullweber): Is there a better way for counters without preferences?
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

void BookmarkCounter::CountBookmarks(BookmarkModel* bookmark_model) {
  int count = CountBookmarksFromNode(bookmark_model->bookmark_bar_node()) +
              CountBookmarksFromNode(bookmark_model->other_node()) +
              CountBookmarksFromNode(bookmark_model->mobile_node());
  ReportResult(count);
}

}  // namespace browsing_data
