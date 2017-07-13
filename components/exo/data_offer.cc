// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_offer.h"

namespace exo {

DataOfferDelegate::~DataOfferDelegate() = default;

DataOffer::DataOffer(std::unique_ptr<DataOfferDelegate> delegate)
    : delegate_(std::move(delegate)) {}
DataOffer::~DataOffer() = default;

void DataOffer::Accept(uint32_t serial, const std::string& mime_type) {
  NOTIMPLEMENTED();
}
void DataOffer::Receive(const std::string& mime_type, base::ScopedFD fd) {
  NOTIMPLEMENTED();
}
void DataOffer::Finish() {
  NOTIMPLEMENTED();
}
void DataOffer::SetActions(uint32_t dnd_actions, uint32_t preferred_action) {
  NOTIMPLEMENTED();
}

}  // namespace exo
