// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/elapsed_timer.h"
#include "extensions/browser/api/declarative_net_request/flat/extension_ruleset_generated.h"
#include "extensions/browser/api/declarative_net_request/utils.h"
#include "extensions/common/api/declarative_net_request/utils.h"

namespace extensions {
namespace declarative_net_request {
namespace flat_rule = url_pattern_index::flat;

bool RulesetMatcher::ShouldBlockRequest(const GURL& url,
                                        const url::Origin& first_party_origin,
                                        flat_rule::ElementType element_type,
                                        bool is_third_party) const {
  SCOPED_UMA_HISTOGRAM_TIMER(
      "DeclarativeNetRequest.ShouldBlockRequest_SingleExtension");

  return !!blacklist_matcher_.FindMatch(url, first_party_origin, element_type,
                                        flat_rule::ActivationType_NONE,
                                        is_third_party,
                                        false /*disable_generic_rules*/) &&
         !whitelist_matcher_.FindMatch(url, first_party_origin, element_type,
                                       flat_rule::ActivationType_NONE,
                                       is_third_party,
                                       false /*disable_generic_rules*/);
}

bool RulesetMatcher::ShouldRedirectRequest(
    const GURL& url,
    const url::Origin& first_party_origin,
    flat_rule::ElementType element_type,
    bool is_third_party,
    GURL* redirect_url) const {
  // TODO(crbug.com/696822): Implement support for redirect request matching.
  NOTIMPLEMENTED();

  return false;
}

// static
RulesetMatcher::LoadRulesetResult RulesetMatcher::CreateVerifiedMatcher(
    const base::FilePath& indexed_ruleset_path,
    int ruleset_checksum,
    std::unique_ptr<RulesetMatcher>* matcher) {
  DCHECK(matcher);
  DCHECK(IsAPIAvailable());
  base::AssertBlockingAllowed();

  base::ElapsedTimer timer;

  if (!base::PathExists(indexed_ruleset_path))
    return LOAD_ERROR_INVALID_PATH;

  scoped_refptr<base::SequencedTaskRunner> deleter_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});
  // Not using (Make/Wrap)Unique since MemoryMappedRuleset has a custom
  // deleter.
  // COMMENT: What's the difference between OnTaskRunnerDeleter and just posting
  // a task to delete from the destructor? If this task fails, the file won't be
  // unmapped?
  // COMMENT: Any other CONS of using a memory mapped file? Other alternative is
  // to read all the data and keep it in memory.
  MemoryMappedRuleset ruleset(
      new base::MemoryMappedFile,
      base::OnTaskRunnerDeleter(std::move(deleter_task_runner)));

  if (!ruleset->Initialize(indexed_ruleset_path,
                           base::MemoryMappedFile::READ_ONLY)) {
    return LOAD_ERROR_MEMORY_MAP;
  }

  // This guarantees that no memory access will end up outside the buffer.
  if (!IsValidRulesetData(ruleset->data(), ruleset->length(),
                          ruleset_checksum)) {
    return LOAD_ERROR_RULESET_VERIFICATION;
  }

  UMA_HISTOGRAM_TIMES(
      "Extensions.DeclarativeNetRequest.CreateVerifiedMatcherDuration",
      timer.Elapsed());

  // Using WrapUnique instead of MakeUnique since this class has a private
  // constructor.
  *matcher = base::WrapUnique(new RulesetMatcher(std::move(ruleset)));
  return LOAD_SUCCESS;
}

RulesetMatcher::RulesetMatcher(MemoryMappedRuleset ruleset)
    : ruleset_(std::move(ruleset)),
      root_(flat::GetExtensionIndexedRuleset(ruleset_->data())),
      blacklist_matcher_(root_->blacklist_index()),
      whitelist_matcher_(root_->whitelist_index()),
      redirect_matcher_(root_->redirect_index()),
      extension_metadata_(root_->extension_metadata()) {}

}  // namespace declarative_net_request
}  // namespace extensions
