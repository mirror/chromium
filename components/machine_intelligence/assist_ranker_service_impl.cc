// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/machine_intelligence/assist_ranker_service_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "components/machine_intelligence/binary_classifier_predictor.h"
#include "components/machine_intelligence/ranker_model_loader_impl.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace machine_intelligence {

AssistRankerServiceImpl::AssistRankerServiceImpl(
    base::FilePath base_path,
    net::URLRequestContextGetter* url_request_context_getter)
    : url_request_context_getter_(url_request_context_getter),
      base_path_(std::move(base_path)) {}

AssistRankerServiceImpl::~AssistRankerServiceImpl() {}

std::unique_ptr<BinaryClassifierPredictor>
AssistRankerServiceImpl::FetchBinaryClassifierPredictor(
    GURL model_url,
    const std::string& model_filename,
    const std::string& uma_prefix) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto predictor = base::MakeUnique<BinaryClassifierPredictor>();
  auto model_loader = base::MakeUnique<RankerModelLoaderImpl>(
      base::Bind(&BinaryClassifierPredictor::ValidateModel),
      base::Bind(&BinaryClassifierPredictor::OnModelAvailable,
                 base::Unretained(predictor.get())),
      url_request_context_getter_.get(), GetModelPath(model_filename),
      model_url, uma_prefix);
  predictor->LoadModel(std::move(model_loader));
  return predictor;
}

base::FilePath AssistRankerServiceImpl::GetModelPath(
    const std::string& model_filename) {
  if (base_path_.empty())
    return base::FilePath();
  return base_path_.AppendASCII(model_filename);
}

}  // namespace machine_intelligence
