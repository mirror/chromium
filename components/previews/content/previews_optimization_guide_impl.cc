// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/content/previews_optimization_guide_impl.h"

#include "base/bind.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace previews {

PreviewsOptimizationGuideImpl::Hints::Hints() {}

PreviewsOptimizationGuideImpl::Hints::~Hints() {}

// static
std::unique_ptr<PreviewsOptimizationGuideImpl::Hints>
PreviewsOptimizationGuideImpl::Hints::CreateFromConfig(
    const optimization_guide::proto::Configuration& config) {
  std::unique_ptr<Hints> hints(new Hints());
  url_matcher::URLMatcherConditionSet::ID id = 0;
  url_matcher::URLMatcherConditionFactory* condition_factory =
      hints->url_matcher_.condition_factory();
  url_matcher::URLMatcherConditionSet::Vector all_conditions;
  std::set<std::string> seen_host_suffixes;
  for (const auto hint : config.hints()) {
    if (hint.key_representation() != optimization_guide::proto::HOST_SUFFIX) {
      continue;
    }
    DCHECK(!hint.key().empty());
    if (hint.key().empty()) {
      continue;
    }
    auto seen_host_suffixes_iter = seen_host_suffixes.find(hint.key());
    DCHECK(seen_host_suffixes_iter == seen_host_suffixes.end());
    if (seen_host_suffixes_iter != seen_host_suffixes.end()) {
      DLOG(WARNING) << "Received config with duplicate key";
      continue;
    }
    seen_host_suffixes.insert(hint.key());
    for (const auto optimization : hint.whitelisted_optimizations()) {
      if (optimization.optimization_type() ==
          optimization_guide::proto::NOSCRIPT) {
        url_matcher::URLMatcherCondition condition =
            condition_factory->CreateHostSuffixCondition(hint.key());
        all_conditions.push_back(new url_matcher::URLMatcherConditionSet(
            id, std::set<url_matcher::URLMatcherCondition>{condition}));
        hints->whitelist_[id] = std::set<PreviewsType>{PreviewsType::NOSCRIPT};
        id++;
        break;
      }
    }
  }
  hints->url_matcher_.AddConditionSets(all_conditions);
  return hints;
}

bool PreviewsOptimizationGuideImpl::Hints::IsWhitelisted(const GURL& url,
                                                         PreviewsType type) {
  std::set<url_matcher::URLMatcherConditionSet::ID> matches =
      url_matcher_.MatchURL(url);
  for (const auto& match : matches) {
    const auto whitelist_iter = whitelist_.find(match);
    if (whitelist_iter == whitelist_.end()) {
      continue;
    }
    const auto& whitelisted_previews = whitelist_iter->second;
    std::set<PreviewsType>::iterator found = whitelisted_previews.find(type);
    // TODO(crbug.com/783237): Make sure this checks for granularity of matched
    // condition when more than one host suffix is matched.
    return found != whitelisted_previews.end();
  }
  return false;
}

PreviewsOptimizationGuideImpl::PreviewsOptimizationGuideImpl(
    optimization_guide::OptimizationGuideService* optimization_guide_service,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : optimization_guide_service_(optimization_guide_service),
      io_task_runner_(io_task_runner),
      background_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND})),
      io_weak_ptr_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(optimization_guide_service_);
  optimization_guide_service_->AddObserver(this);
}

PreviewsOptimizationGuideImpl::~PreviewsOptimizationGuideImpl() {
  optimization_guide_service_->RemoveObserver(this);
}

bool PreviewsOptimizationGuideImpl::IsWhitelisted(
    const net::URLRequest& request,
    PreviewsType type) const {
  return hints_->IsWhitelisted(request.url(), type);
}

void PreviewsOptimizationGuideImpl::OnHintsProcessed(
    const optimization_guide::proto::Configuration& config) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  base::PostTaskAndReplyWithResult(
      background_task_runner_.get(), FROM_HERE,
      base::BindOnce(&PreviewsOptimizationGuideImpl::Hints::CreateFromConfig,
                     config),
      base::BindOnce(&PreviewsOptimizationGuideImpl::UpdateHints,
                     io_weak_ptr_factory_.GetWeakPtr()));
}

void PreviewsOptimizationGuideImpl::UpdateHints(std::unique_ptr<Hints> hints) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  hints_ = std::move(hints);
}

}  // namespace previews
