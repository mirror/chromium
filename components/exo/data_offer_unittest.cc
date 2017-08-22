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
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/data_device.h"
#include "components/exo/data_offer_delegate.h"
#include "components/exo/test/exo_test_base.h"
#include "components/exo/test/test_data_offer_delegate.h"
#include "ui/base/dragdrop/os_exchange_data.h"

namespace exo {
namespace {

using DataOfferTest = test::ExoTestBase;

TEST_F(DataOfferTest, SendEvents) {
  ui::OSExchangeData data;
  data.SetString(base::string16(base::ASCIIToUTF16("Test data")));

  base::flat_set<DndAction> source_actions;
  source_actions.insert(DndAction::kCopy);
  source_actions.insert(DndAction::kMove);

  std::unique_ptr<TestDataOfferDelegate> delegate =
      base::MakeUnique<TestDataOfferDelegate>();
  std::unique_ptr<DataOffer> data_offer =
      base::MakeUnique<DataOffer>(delegate.get());

  EXPECT_EQ(0u, delegate->mime_types().size());
  EXPECT_EQ(0u, delegate->source_actions().size());
  EXPECT_EQ(DndAction::kNone, delegate->dnd_action());

  data_offer->SetDropData(nullptr, data);
  data_offer->SetSourceActions(source_actions);
  data_offer->SetActions(base::flat_set<DndAction>(), DndAction::kMove);

  EXPECT_EQ(1u, delegate->mime_types().size());
  EXPECT_EQ("text/plain", delegate->mime_types()[0]);
  EXPECT_EQ(2u, delegate->source_actions().size());
  EXPECT_EQ(1u, delegate->source_actions().count(DndAction::kCopy));
  EXPECT_EQ(1u, delegate->source_actions().count(DndAction::kMove));
  EXPECT_EQ(DndAction::kMove, delegate->dnd_action());
}

}  // namespace
}  // namespace exo
