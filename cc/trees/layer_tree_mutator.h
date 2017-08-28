// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_LAYER_TREE_MUTATOR_H_
#define CC_TREES_LAYER_TREE_MUTATOR_H_

#include "base/callback_forward.h"
#include "base/time/time.h"
#include "cc/cc_export.h"

namespace cc {

struct AnimatorInputState {
 public:
  int animation_player_id_ = 0;
  std::string name;
  base::TimeTicks current_time_;
};

using AnimatorsInputState = std::vector<AnimatorInputState>;

struct AnimatorOutputState {
  int animation_player_id_ = 0;
  base::TimeDelta local_time_;
};

using AnimatorsOutputState = std::vector<AnimatorOutputState>;

class LayerTreeMutatorClient {
 public:
  // Called when mutator needs to request another frame or update its output.
  //
  // |needs_mutate|: Determines if mutator needs another impl frame. We
  // couldn't, for example, just assume that we will "always mutate" and early-
  // out in the mutator if there was nothing to do because we won't always be
  // producing impl frames.
  // |output_state|: Most recent output of the mutator.
  virtual void SetMutationUpdate(
      bool needs_mutate,
      std::unique_ptr<AnimatorsOutputState> output_state) = 0;
};

class CC_EXPORT LayerTreeMutator {
 public:
  virtual ~LayerTreeMutator() {}

  // TODO(majidvp): get rid of |now| since each animator now receives its own
  // current time.
  virtual void Mutate(base::TimeTicks now,
                      std::unique_ptr<AnimatorsInputState> input_state) = 0;
  virtual void SetClient(LayerTreeMutatorClient* client) = 0;

  // Returns a callback which is responsible for applying layer tree mutations
  // to DOM elements.
  virtual base::Closure TakeMutations() = 0;
};

}  // namespace cc

#endif  // CC_TREES_LAYER_TREE_MUTATOR_H_
