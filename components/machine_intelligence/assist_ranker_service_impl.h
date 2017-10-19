// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_IMPL_H_
#define COMPONENTS_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_IMPL_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "components/machine_intelligence/assist_ranker_service.h"
#include "components/machine_intelligence/ranker_model_loader.h"

class GURL;

namespace machine_intelligence {

class RankerModelLoader;

class AssistRankerServiceImpl : public AssistRankerService {
 public:
  AssistRankerServiceImpl(
      base::FilePath base_path,
      net::URLRequestContextGetter* url_request_context_getter);
  ~AssistRankerServiceImpl() override;

  // AssistRankerService...
  std::unique_ptr<RankerModelLoader> InitializeModel(
      GURL model_url,
      std::string model_filename,
      std::string UMAPrefix,
      AssistRankerService::OnModelAvailableCallback
          on_model_available_callback);

 private:
  base::FilePath GetModelPath(const std::string& model_filename);

  // Used to sanity check the threading of this ranker.
  base::SequenceChecker sequence_checker_;

  // Request Context Getter used for RankerURLFetcher.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Base path where models are stored.
  const base::FilePath base_path_;

  base::WeakPtrFactory<AssistRankerServiceImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AssistRankerServiceImpl);
};

}  // namespace machine_intelligence

#endif  // COMPONENTS_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_IMPL_H_
