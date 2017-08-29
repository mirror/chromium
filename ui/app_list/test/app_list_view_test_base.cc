// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/test/app_list_view_test_base.h"

#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/aura/window.h"

namespace app_list {

namespace test {

AppListViewTestBase::AppListViewTestBase() = default;
AppListViewTestBase::~AppListViewTestBase() = default;

void AppListViewTestBase::SetUp() {
  views::ViewsTestBase::SetUp();

  delegate_.reset(new AppListTestViewDelegate);
  app_list_view_ = new AppListView(delegate_.get());
  gfx::NativeView parent = GetContext();
  app_list_view_->Initialize(parent, 0, false, false);
  // TODO(warx): remove MaybeSetAnchorPoint setup when bubble launcher is
  // removed from code base.
  app_list_view_->MaybeSetAnchorPoint(
      parent->GetBoundsInRootWindow().CenterPoint());
  app_list_view_->GetWidget()->Show();
  contents_view_ = app_list_view_->app_list_main_view()->contents_view();
}

void AppListViewTestBase::TearDown() {
  app_list_view_->GetWidget()->Close();
  views::ViewsTestBase::TearDown();
}

}  // namespace test

}  // namespace app_list
