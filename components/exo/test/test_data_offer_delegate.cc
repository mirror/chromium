// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/test/test_data_offer_delegate.h"

namespace exo {

TestDataOfferDelegate::TestDataOfferDelegate() = default;

TestDataOfferDelegate::~TestDataOfferDelegate() = default;

void TestDataOfferDelegate::OnDataOfferDestroying(DataOffer* offer) {}

void TestDataOfferDelegate::OnOffer(const std::string& mime_type) {
  mime_types_.push_back(mime_type);
}

void TestDataOfferDelegate::OnSourceActions(
    const base::flat_set<DndAction>& source_actions) {
  source_actions_ = source_actions;
}

void TestDataOfferDelegate::OnAction(DndAction dnd_action) {
  dnd_action_ = dnd_action;
}

}  // namespace exo
