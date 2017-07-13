// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/ruleset_matcher.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_restrictions.h"
#include "extensions/common/api/declarative_net_request/flat/extension_ruleset_generated.h"
#include "extensions/common/api/declarative_net_request/utils.h"

namespace extensions {
namespace declarative_net_request {
namespace flat_rule = ::url_pattern_index::flat;

bool RulesetMatcher::ShouldBlockRequest(const GURL& url,
                                        const url::Origin& first_party_origin,
                                        flat_rule::ElementType element_type,
                                        bool is_third_party) const {
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
  // TODO(karandeepb): Implement support for redirect request matching.
  return false;
}

// static
RulesetMatcher::LoadRulesetResult RulesetMatcher::CreateVerifiedMatcher(
    const base::FilePath& indexed_ruleset_path,
    std::unique_ptr<RulesetMatcher>* matcher) {
  DCHECK(matcher);
  base::ThreadRestrictions::AssertIOAllowed();

  const base::TimeTicks start = base::TimeTicks::Now();

  if (!base::PathExists(indexed_ruleset_path))
    return LOAD_ERROR_INVALID_PATH;

  // Not using [Make/Wrap]Unique since MemoryMappedRuleset has a custom deleter.
  MemoryMappedRuleset ruleset(new base::MemoryMappedFile());
  if (!ruleset->Initialize(indexed_ruleset_path,
                           base::MemoryMappedFile::READ_ONLY)) {
    return LOAD_ERROR_MEMORY_MAP;
  }

  // TODO(karandeepb): Also store and verify file hash.

  // This guarantees that no memory access will end up outside the buffer.
  flatbuffers::Verifier verifier(ruleset->data(), ruleset->length());
  if (!flat::VerifyExtensionIndexedRulesetBuffer(verifier))
    return LOAD_ERROR_RULESET_VERIFICATION;

  UMA_HISTOGRAM_TIMES(
      "Extensions.DeclarativeNetRequest.CreateVerifiedMatcherDuration",
      base::TimeTicks::Now() - start);

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
      extension_metdata_(root_->extension_metdata()) {
  DCHECK(IsAPIAvailable());
}

}  // namespace declarative_net_request
}  // namespace extensions
