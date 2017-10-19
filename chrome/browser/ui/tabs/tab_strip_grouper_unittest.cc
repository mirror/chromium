// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_grouper.h"

#include "chrome/browser/ui/tabs/tab_strip_grouper_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::WebContents;

namespace {

enum class AddRemoveOp { ADD, REMOVE };

struct AddRemove {
  AddRemove() = default;
  AddRemove(AddRemoveOp in_op, int in_begin, int in_count)
      : op(in_op), begin(in_begin), count(in_count) {}

  bool operator==(const AddRemove& other) const {
    return op == other.op && begin == other.begin && count == other.count;
  }

  AddRemoveOp op = AddRemoveOp::ADD;
  int begin = 0;
  int count = 0;
};

class TestGrouperObserver : public TabStripGrouperObserver {
 public:
  TestGrouperObserver() {}
  ~TestGrouperObserver() {}

  void ClearOps() { ops_.clear(); }

  void ExpectOpsAreAndClear(std::initializer_list<AddRemove> expected) {
    EXPECT_EQ(expected.size(), ops_.size());
    if (expected.size() == ops_.size()) {
      for (size_t i = 0; i < ops_.size(); i++)
        EXPECT_EQ(*(expected.begin() + i), ops_[i]);
    }
    ops_.clear();
  }

  std::vector<AddRemove> GetAndClearOps() {
    std::vector<AddRemove> ret = std::move(ops_);
    ops_.clear();
    return ret;
  }

 private:
  void ItemsInsertedAt(TabStripGrouper* grouper,
                       int begin_index,
                       int count) override {
    AddRemove op;
    op.op = AddRemoveOp::ADD;
    op.begin = begin_index;
    op.count = count;
    ops_.push_back(op);
  }
  void ItemsClosingAt(TabStripGrouper* grouper,
                      int begin_index,
                      int count) override {
    AddRemove op;
    op.op = AddRemoveOp::REMOVE;
    op.begin = begin_index;
    op.count = count;
    ops_.push_back(op);
  }

  std::vector<AddRemove> ops_;
};

class TabStripGrouperTest : public ChromeRenderViewHostTestHarness {
 public:
  TabStripGrouperTest()
      : model_delegate_(),
        model_(&model_delegate_, profile()),
        grouper_(&model_) {
    grouper_.AddObserver(&observer_);
  }

  WebContents* CreateWebContents() {
    return WebContents::Create(WebContents::CreateParams(profile()));
  }

 protected:
  TestTabStripModelDelegate model_delegate_;
  TabStripModel model_;
  TabStripGrouper grouper_;
  TestGrouperObserver observer_;
};

}  // namespace

TEST_F(TabStripGrouperTest, InsertRemove) {
  WebContents* contents1 = CreateWebContents();
  model_.AppendWebContents(contents1, true);
  model_.CloseWebContentsAt(0, TabStripModel::CLOSE_USER_GESTURE |
                                   TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);

  observer_.ExpectOpsAreAndClear({AddRemove(AddRemoveOp::ADD, 1, 2),
                                  AddRemove(AddRemoveOp::REMOVE, 1, 2)});
}

// Test inserting an item at the end of a group. Say it goes into the group,
// then say it doesn't.
