// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_source.h"

#include "base/bind.h"
#include "base/test/scoped_task_environment.h"
#include "components/exo/data_source_delegate.h"
#include "components/exo/test/exo_test_base.h"

namespace exo {
namespace {

class DataSourceTest : public test::ExoTestBase {
 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

class TestDataSourceDelegate : public DataSourceDelegate {
 public:
  TestDataSourceDelegate() {}
  ~TestDataSourceDelegate() override {}

  // Overridden from DataSourceDelegate:
  void OnDataSourceDestroying(DataSource* source) override {}
  void OnTarget(const std::string& mime_type) override {}
  void OnSend(const std::string& mime_type, base::ScopedFD fd) override {}
  void OnCancelled() override {}
  void OnDndDropPerformed() override {}
  void OnDndFinished() override {}
  void OnAction(DndAction dnd_action) override {}
};

TEST_F(DataSourceTest, TODO) {
  TestDataSourceDelegate delegate;
  DataSource data_source(&delegate);

  data_source.ReadData(
      base::BindOnce([](std::vector<uint8_t> data, exo::DataSource* source) {

      }));
}

}  // namespace
}  // namespace exo
