// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_OFFER_H_
#define COMPONENTS_EXO_DATA_OFFER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "ui/base/class_property.h"

namespace exo {

class DataOfferDelegate;
enum class DndAction;

// Object representing transferred data offered to a client.
class DataOffer : public ui::PropertyHandler {
 public:
  DataOffer(DataOfferDelegate* delegate,
            std::vector<std::string> mime_types,
            const base::flat_set<DndAction>& source_actions,
            DndAction dnd_action);
  ~DataOffer();

  // Accepts one of the offered mime types.
  void Accept(const std::string& mime_type);

  // Requests that the data is transferred. |fd| is a file descriptor for data
  // transfer.
  void Receive(const std::string& mime_type, base::ScopedFD fd);

  // Called when the client is no longer using the data offer object.
  void Finish();

  // Sets the available/preferred drag-and-drop actions.
  void SetActions(const base::flat_set<DndAction>& dnd_actions,
                  DndAction preferred_action);

  // Sends events to the client to notify the current member values of
  // DataOffer.
  void SendEvents();

  DndAction dnd_action() { return dnd_action_; }

 private:
  DataOfferDelegate* const delegate_;
  std::vector<std::string> mime_types_;
  base::flat_set<DndAction> source_actions_;
  DndAction dnd_action_;

  DISALLOW_COPY_AND_ASSIGN(DataOffer);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_OFFER_H_
