// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_TEST_APP_LIST_VIEW_TEST_BASE_H_
#define UI_APP_LIST_TEST_APP_LIST_VIEW_TEST_BASE_H_

#include "base/macros.h"
#include "ui/views/test/views_test_base.h"

namespace app_list {

class AppListView;
class ContentsView;

namespace test {

class AppListTestViewDelegate;

class AppListViewTestBase : public views::ViewsTestBase {
 public:
  AppListViewTestBase();
  ~AppListViewTestBase() override;

  // testing::Test overrides:
  void SetUp() override;
  void TearDown() override;

 protected:
  AppListView* app_list_view() const { return app_list_view_; }
  ContentsView* contents_view() const { return contents_view_; }
  AppListTestViewDelegate* delegate() const { return delegate_.get(); }

 private:
  AppListView* app_list_view_ = nullptr;   // Owned by native widget.
  ContentsView* contents_view_ = nullptr;  // Owned by views hierarchy.
  std::unique_ptr<AppListTestViewDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppListViewTestBase);
};

}  // namespace test

}  // namespace app_list

#endif  // UI_APP_LIST_TEST_APP_LIST_VIEW_TEST_BASE_H_
