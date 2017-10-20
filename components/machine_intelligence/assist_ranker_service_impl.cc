// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/machine_intelligence/assist_ranker_service_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/metrics_hashes.h"
#include "base/strings/string_util.h"
#include "base/task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/machine_intelligence/proto/ranker_model.pb.h"
#include "components/machine_intelligence/ranker_model.h"
#include "components/machine_intelligence/ranker_model_loader.h"
#include "components/machine_intelligence/ranker_predictor.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace machine_intelligence {

AssistRankerServiceImpl::AssistRankerServiceImpl(
    base::FilePath base_path,
    net::URLRequestContextGetter* url_request_context_getter)
    : url_request_context_getter_(url_request_context_getter),
      base_path_(std::move(base_path)),
      weak_ptr_factory_(this) {}

std::unique_ptr<RankerPredictor> AssistRankerServiceImpl::FetchModel(
    GURL model_url,
    std::string model_filename,
    std::string uma_prefix) {
  auto predictor = base::MakeUnique<RankerPredictor>();
  predictor->LoadModel(model_url, GetModelPath(model_filename), uma_prefix,
                       url_request_context_getter_.get());
  return predictor;
}

AssistRankerServiceImpl::~AssistRankerServiceImpl() {}

base::FilePath AssistRankerServiceImpl::GetModelPath(
    const std::string& model_filename) {
  if (base_path_.empty())
    return base::FilePath();
  return base_path_.AppendASCII(model_filename);
}

}  // namespace machine_intelligence
