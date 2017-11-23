// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ASSIST_RANKER_ASSIST_RANKER_SERVICE_H_
#define COMPONENTS_ASSIST_RANKER_ASSIST_RANKER_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

namespace assist_ranker {

class BinaryClassifierPredictor;
class PredictorConfig;

// TODO(crbug.com/778468) : Refactor this so that the service owns the predictor
// objects and enforce model uniqueness through internal registration in order
// to avoid cache collisions.
//
// Service that provides Predictor objects.
class AssistRankerService : public KeyedService {
 public:
  AssistRankerService() = default;

  // Returns a binary classification model. |model_name| is the filename of
  // the cached model and unique identifier of this predictor. This model_name
  // must have a pre-defined PredictorConfig in the config map.
  // FIXME: fix this comment.
  virtual base::WeakPtr<BinaryClassifierPredictor>
  FetchBinaryClassifierPredictor(const std::string& model_name) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AssistRankerService);
};

}  // namespace assist_ranker

#endif  // COMPONENTS_ASSIST_RANKER_ASSIST_RANKER_SERVICE_H_
