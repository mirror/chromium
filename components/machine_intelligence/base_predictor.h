// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MACHINE_INTELLIGENCE_BASE_PREDICTOR_H_
#define COMPONENTS_MACHINE_INTELLIGENCE_BASE_PREDICTOR_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "components/machine_intelligence/ranker_model_loader.h"

class GURL;

namespace net {
class URLRequestContextGetter;
}

namespace machine_intelligence {

class RankerModel;

// Predictors are objects that provide an interface for prediction, as well as
// encapsulate the logic for loading the model. Sub-classes of BasePredictor
// implement an interface that depends on the nature of the suported model.
class BasePredictor {
 public:
  BasePredictor();
  virtual ~BasePredictor();

  // Loads a model and make it available for prediction.
  // |model_url| is the unique identifier that corresponds to the model's origin
  // URL. The RankerModelLoader will first check if the given model is cached on
  // disk at |model_path|. If not, it will trigger a download and cache it to
  // |model_path|. |uma_prefix| is used to log UMA histograms related to this
  // model. |url_request_context_getter| is passed on to the RankerModelLoader.
  void LoadModel(GURL model_url,
                 base::FilePath model_path,
                 std::string uma_prefix,
                 net::URLRequestContextGetter* url_request_context_getter);

  // Returns wheter or not the model is ready for prediction.
  bool IsReady();

 protected:
  // The model used for prediction.
  std::unique_ptr<RankerModel> ranker_model_;

  // Called when the RankerModelLoader has finished loading the model. Returns
  // true only if the model was succesfully loaded and is ready to predict.
  virtual bool Initialize();

 private:
  bool is_ready_;
  static RankerModelStatus ValidateModel(const RankerModel& model);
  void OnModelAvailable(std::unique_ptr<RankerModel> model);
  std::unique_ptr<RankerModelLoader> model_loader_;
  base::WeakPtrFactory<BasePredictor> weak_ptr_factory_;
};

}  // namespace machine_intelligence
#endif  // COMPONENTS_MACHINE_INTELLIGENCE_BASE_PREDICTOR_H_
