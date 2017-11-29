// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_VARIATIONS_ID_COLLECTION_H_
#define COMPONENTS_VARIATIONS_VARIATIONS_ID_COLLECTION_H_

#include <set>
#include <string>

#include "base/metrics/field_trial.h"
#include "components/variations/variations_associated_data.h"

namespace variations {

// Watches finalization of trials that may have a variation id for the given
// key. Maintains a list of ids for the given key, and invokes a callback every
// time a new id is found. Does not invoke the callback for ids for trails that
// were finalized prior to the construction of a VariationsIdCollection
// instance. Is not thread safe.
class VariationsIdCollection : public base::FieldTrialList::Observer {
 public:
  VariationsIdCollection(
      variations::IDCollectionKey collection_key,
      base::RepeatingCallback<void(variations::VariationID)> new_id_callback);

  ~VariationsIdCollection() override;

  void OnFieldTrialGroupFinalized(const std::string& trial_name,
                                  const std::string& group_name) override;

  const std::set<variations::VariationID>& GetIds();

 private:
  void CheckForVariation(const std::string& trial_name,
                         const std::string& group_name);

  variations::IDCollectionKey collection_key_;
  base::RepeatingCallback<void(variations::VariationID)> new_id_callback_;
  std::set<variations::VariationID> id_set_;
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_VARIATIONS_ID_COLLECTION_H_
