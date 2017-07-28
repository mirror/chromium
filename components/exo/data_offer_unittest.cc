// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_offer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "components/exo/data_device.h"
#include "components/exo/data_offer_delegate.h"
#include "components/exo/test/exo_test_base.h"

namespace exo {
namespace {

using DataOfferTest = test::ExoTestBase;

class TestDataOfferDelegate : public DataOfferDelegate {
 public:
  TestDataOfferDelegate() : dnd_action_(DndAction::kNone) {}

  // Called at the top of the data device's destructor, to give observers a
  // chance to remove themselves.
  void OnDataOfferDestroying(DataOffer* offer) override {}

  // Called when |mime_type| is offered by the client.
  void OnOffer(const std::string& mime_type) override {
    mime_types_.push_back(mime_type);
  }

  // Called when possible |source_actions| is offered by the client.
  void OnSourceActions(
      const base::flat_set<DndAction>& source_actions) override {
    source_actions_ = source_actions;
  }

  // Called when current |action| is offered by the client.
  void OnAction(DndAction dnd_action) override { dnd_action_ = dnd_action; }

  const std::vector<std::string>& mime_types() const { return mime_types_; }
  const base::flat_set<DndAction>& source_actions() const {
    return source_actions_;
  }
  DndAction dnd_action() const { return dnd_action_; }

 private:
  std::vector<std::string> mime_types_;
  base::flat_set<DndAction> source_actions_;
  DndAction dnd_action_;

  DISALLOW_COPY_AND_ASSIGN(TestDataOfferDelegate);
};

TEST_F(DataOfferTest, SendEvents) {
  std::vector<std::string> mime_types;
  mime_types.push_back("test/mimetype1");
  mime_types.push_back("test/mimetype2");

  base::flat_set<DndAction> source_actions;
  source_actions.insert(DndAction::kCopy);
  source_actions.insert(DndAction::kMove);

  std::unique_ptr<TestDataOfferDelegate> delegate =
      base::MakeUnique<TestDataOfferDelegate>();
  std::unique_ptr<DataOffer> data_offer = base::MakeUnique<DataOffer>(
      delegate.get(), mime_types, source_actions, DndAction::kMove);

  EXPECT_EQ(0u, delegate->mime_types().size());
  EXPECT_EQ(0u, delegate->source_actions().size());
  EXPECT_EQ(DndAction::kNone, delegate->dnd_action());

  data_offer->SendEvents();

  EXPECT_EQ(2u, delegate->mime_types().size());
  EXPECT_EQ("test/mimetype1", delegate->mime_types()[0]);
  EXPECT_EQ("test/mimetype2", delegate->mime_types()[1]);
  EXPECT_EQ(2u, delegate->source_actions().size());
  EXPECT_EQ(1u, delegate->source_actions().count(DndAction::kCopy));
  EXPECT_EQ(1u, delegate->source_actions().count(DndAction::kMove));
  EXPECT_EQ(DndAction::kMove, delegate->dnd_action());
}

}  // namespace
}  // namespace exo
