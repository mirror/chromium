// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_OFFER_H_
#define COMPONENTS_EXO_DATA_OFFER_H_

#include <cstdint>
#include <string>

#include "base/files/scoped_file.h"
#include "base/macros.h"

namespace exo {

class DataOfferDelegate;

// Object representing transferred data offered to a client.
// Requests and events are compatible wl_data_offer. See wl_data_offer for
// details.
class DataOffer {
 public:
  explicit DataOffer(DataOfferDelegate* delegate);
  ~DataOffer();

  // Accepts one of the offered mime types
  void Accept(uint32_t serial, const std::string& mime_type);

  // Requests that the data is transferred
  void Receive(const std::string& mime_type, base::ScopedFD fd);

  // Notifies that the offer will no longer be used
  void Finish();

  // Sets the available/preferred drag-and-drop actions
  void SetActions(uint32_t dnd_actions, uint32_t preferred_action);

  const DataOfferDelegate* delegate() const { return delegate_; }

 private:
  DataOfferDelegate* const delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataOffer);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_OFFER_H_
