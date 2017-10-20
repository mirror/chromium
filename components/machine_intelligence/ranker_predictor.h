// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MACHINE_INTELLIGENCE_RANKER_PREDICTOR_H_
#define COMPONENTS_MACHINE_INTELLIGENCE_RANKER_PREDICTOR_H_

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

class RankerPredictor {
 public:
  RankerPredictor();
  virtual ~RankerPredictor();
  void LoadModel(GURL model_url,
                 base::FilePath model_path,
                 std::string uma_prefix,
                 net::URLRequestContextGetter* url_request_context_getter);

  bool IsReady();

 protected:
  std::unique_ptr<RankerModel> ranker_model_;

 private:
  // Called when the model is available.
  virtual void Initialize();
  bool is_ready_ = false;
  static RankerModelStatus ValidateModel(const RankerModel& model);
  void OnModelAvailable(std::unique_ptr<RankerModel> model);
  std::unique_ptr<RankerModelLoader> model_loader_;
  base::WeakPtrFactory<RankerPredictor> weak_ptr_factory_;
};

}  // namespace machine_intelligence
#endif  // COMPONENTS_MACHINE_INTELLIGENCE_RANKER_PREDICTOR_H_
