// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_H_
#define COMPONENTS_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;

namespace machine_intelligence {

class BinaryClassifierPredictor;

// Service that provides Predictor objects.
class AssistRankerService : public KeyedService {
 public:
  AssistRankerService() = default;

  // TODO(hamelphi): Make this a templated method.
  // Returns a binary classification model.
  virtual std::unique_ptr<BinaryClassifierPredictor>
  FetchBinaryClassifierPredictor(GURL model_url,
                                 std::string model_filename,
                                 std::string uma_prefix) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AssistRankerService);
};

}  // namespace machine_intelligence

#endif  // COMPONENTS_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_H_
