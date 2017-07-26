// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_offer.h"

#include "components/exo/data_offer_delegate.h"

namespace exo {

DataOffer::DataOffer(DataOfferDelegate* delegate,
                     std::vector<std::string> mime_types,
                     const base::flat_set<DndAction>& source_actions,
                     DndAction dnd_action)
    : delegate_(delegate),
      mime_types_(mime_types),
      source_actions_(source_actions),
      dnd_action_(dnd_action),
      weak_factory_(this) {}

DataOffer::~DataOffer() {
  delegate_->OnDataOfferDestroying(this);
}

base::WeakPtr<DataOffer> DataOffer::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void DataOffer::Accept(const std::string& mime_type) {
  NOTIMPLEMENTED();
}

void DataOffer::Receive(const std::string& mime_type, base::ScopedFD fd) {
  NOTIMPLEMENTED();
}

void DataOffer::Finish() {
  NOTIMPLEMENTED();
}

void DataOffer::SetActions(const base::flat_set<DndAction>& dnd_actions,
                           DndAction preferred_action) {
  NOTIMPLEMENTED();
}

void DataOffer::SendEvents() {
  for (const std::string& mime_type : mime_types_) {
    delegate_->OnOffer(mime_type);
  }
  delegate_->OnSourceActions(source_actions_);
  delegate_->OnAction(dnd_action_);
}

}  // namespace exo
