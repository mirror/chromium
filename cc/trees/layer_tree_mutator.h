// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_LAYER_TREE_MUTATOR_H_
#define CC_TREES_LAYER_TREE_MUTATOR_H_

#include "base/callback_forward.h"
#include "base/time/time.h"
#include "cc/cc_export.h"

namespace cc {

class LayerTreeMutatorClient {
 public:
  // This is necessary because it forces an impl frame. We couldn't, for
  // example, just assume that we will "always mutate" and early-out in the
  // mutator if there was nothing to do because we won't always be producing
  // impl frames.
  virtual void SetNeedsMutate() = 0;
};

struct AnimatorInputState {
 public:
  int animation_player_id_ = 0;
  base::TimeTicks current_time_;
};

using AnimatorsInputState = std::vector<AnimatorInputState>;

// struct AnimatorOutputState {
//   int animation_player_id_ = 0;
//   base::TimeTicks local_time_;
// }

class CC_EXPORT LayerTreeMutator {
 public:
  virtual ~LayerTreeMutator() {}

  // Returns true if the mutator should be rescheduled.
  // TODO(majidvp): get rid of |now| since each animator now receives its own
  // current time.
  virtual bool Mutate(base::TimeTicks now,
                      std::unique_ptr<AnimatorsInputState>) = 0;
  virtual void SetClient(LayerTreeMutatorClient* client) = 0;

  // Returns a callback which is responsible for applying layer tree mutations
  // to DOM elements.
  virtual base::Closure TakeMutations() = 0;
};

}  // namespace cc

#endif  // CC_TREES_LAYER_TREE_MUTATOR_H_
