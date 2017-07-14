// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_OFFER_DELEGATE_H_
#define COMPONENTS_EXO_DATA_OFFER_DELEGATE_H_

#include <string>

namespace exo {

class DataOffer;

// Provides protocol specific implementations of DataOffer.
class DataOfferDelegate {
 public:
  // Notifies when DataOffer is being destroyed.
  virtual void OnDataOfferDestroying(DataOffer* offer) = 0;

  // Notifies |mime_type| which the data offer object is offering.
  virtual void OnOffer(const std::string& mime_type) = 0;

  // Notifies possible |source_actions| which the data offer object is offering.
  virtual void OnSourceActions(uint32_t source_actions) = 0;

  // Notifies current |action| which the data offer object is offering.
  virtual void OnAction(uint32_t action) = 0;

 protected:
  virtual ~DataOfferDelegate() = default;
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_OFFER_DELEGATE_H_
