// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_OFFER_H_
#define COMPONENTS_EXO_DATA_OFFER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"

namespace exo {

// Provides protocol specific implementations of DataOffer.
class DataOfferDelegate {
 public:
  virtual ~DataOfferDelegate();
  virtual void Offer(const std::string& mime_type) = 0;
  virtual void SourceActions(uint32_t source_actions) = 0;
  virtual void Action(uint32_t action) = 0;
};

// Object representing transferred data offered to a client.
// Requests and events are compatible wl_data_offer. See wl_data_offer for
// details.
class DataOffer {
 public:
  explicit DataOffer(std::unique_ptr<DataOfferDelegate> delegate);

  void Accept(uint32_t serial, const std::string& mime_type);
  void Receive(const std::string& mime_type, base::ScopedFD fd);
  void Finish();
  void SetActions(uint32_t dnd_actions, uint32_t preferred_action);

  const DataOfferDelegate* delegate() const { return delegate_.get(); }

 private:
  std::unique_ptr<DataOfferDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataOffer);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_OFFER_H_
