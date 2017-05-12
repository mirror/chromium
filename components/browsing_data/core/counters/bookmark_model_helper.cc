// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browsing_data/core/counters/bookmark_model_helper.h"

#include "components/bookmarks/browser/bookmark_model.h"

namespace browsing_data {

BookmarkModelHelper::BookmarkModelHelper(
    bookmarks::BookmarkModel* bookmark_model,
    BookmarkModelCallback callback)
    : bookmark_model_(bookmark_model), callback_(std::move(callback)) {
  DCHECK(!bookmark_model_->loaded());
  bookmark_model_->AddObserver(this);
}

BookmarkModelHelper::~BookmarkModelHelper() {
  if (bookmark_model_)
    bookmark_model_->RemoveObserver(this);
}

void BookmarkModelHelper::BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                                              bool ids_reassigned) {
  bookmark_model_ = nullptr;
  model->RemoveObserver(this);
  std::move(callback_).Run(model);
};

void BookmarkModelHelper::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* model) {
  bookmark_model_ = nullptr;
}

}  // namespace browsing_data
