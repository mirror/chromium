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

class RankerModel;
class RankerModelLoader;

// If enabled, downloads a assist ranker model and uses it to perform inference.
class AssistRankerService : public KeyedService {
 public:
  using OnModelAvailableCallback = base::RepeatingCallback<void(
      std::unique_ptr<machine_intelligence::RankerModel>)>;

  AssistRankerService() = default;

  // FIXME: Return a model or predictor object that takes care of loading and
  // parsing and offers an interface for prediction.
  RankerModelLoader InitializeModel(
      GURL model_url,
      std::string model_filename,
      std::string UMAPrefix,
      OnModelAvailableCallback on_model_available_callback);

 private:
  DISALLOW_COPY_AND_ASSIGN(AssistRankerService);
};

}  // namespace machine_intelligence

#endif  // COMPONENTS_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_H_
