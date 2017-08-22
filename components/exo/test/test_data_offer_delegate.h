// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_TEST_DATA_OFFER_DELEGATE_H_
#define COMPONENTS_EXO_TEST_DATA_OFFER_DELEGATE_H_

#include <vector>

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "components/exo/data_device.h"
#include "components/exo/data_offer_delegate.h"

namespace exo {

class TestDataOfferDelegate : public DataOfferDelegate {
 public:
  TestDataOfferDelegate();
  ~TestDataOfferDelegate() override;

  const std::vector<std::string>& mime_types() const { return mime_types_; }
  const base::flat_set<DndAction>& source_actions() const {
    return source_actions_;
  }
  DndAction dnd_action() const { return dnd_action_; }

  // Overriden from DataOfferDelegate:
  void OnDataOfferDestroying(DataOffer* offer) override;
  void OnOffer(const std::string& mime_type) override;
  void OnSourceActions(
      const base::flat_set<DndAction>& source_actions) override;
  void OnAction(DndAction dnd_action) override;

 private:
  std::vector<std::string> mime_types_;
  base::flat_set<DndAction> source_actions_;
  DndAction dnd_action_{DndAction::kNone};

  DISALLOW_COPY_AND_ASSIGN(TestDataOfferDelegate);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_TEST_DATA_OFFER_DELEGATE_H_
