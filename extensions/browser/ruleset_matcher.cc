// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/ruleset_matcher.h"

#include "base/files/file_util.h"
#include "base/files/memory_mapped_file.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_restrictions.h"
#include "extensions/common/api/declarative_net_request/flat/extension_ruleset_generated.h"

namespace extensions {
namespace declarative_net_request {
namespace flat_rule = ::url_pattern_index::flat;

bool RulesetMatcher::ShouldBlockRequest(const GURL& url,
                                        const url::Origin& first_party_origin,
                                        flat_rule::ElementType element_type,
                                        bool is_third_party) const {
  return !!blacklist_matcher_.FindMatch(url, first_party_origin, element_type,
                                        flat_rule::ActivationType_NONE,
                                        is_third_party, false) &&
         !whitelist_matcher_.FindMatch(url, first_party_origin, element_type,
                                       flat_rule::ActivationType_NONE,
                                       is_third_party, false);
}

bool RulesetMatcher::ShouldRedirectRequest(
    const GURL& url,
    const url::Origin& first_party_origin,
    flat_rule::ElementType element_type,
    bool is_third_party,
    GURL* redirect_url) const {
  NOTIMPLEMENTED();
  return false;
}

// static
std::unique_ptr<RulesetMatcher> RulesetMatcher::CreateVerifiedMatcher(
    const base::FilePath& indexed_ruleset_path) {
  base::ThreadRestrictions::AssertIOAllowed();
  base::TimeTicks start = base::TimeTicks::Now();

  // TODO log error uma.
  if (!base::PathExists(indexed_ruleset_path)) {
    NOTREACHED() << "Ruleset file does not exist";
    return nullptr;
  }

  std::unique_ptr<base::MemoryMappedFile,
                  content::BrowserThread::DeleteOnFileThread>
      ruleset_file(new base::MemoryMappedFile());
  if (!ruleset_file->Initialize(indexed_ruleset_path,
                                base::MemoryMappedFile::READ_ONLY)) {
    return nullptr;
  }

  // TODO we should also verify file checksum.
  flatbuffers::Verifier verifier(ruleset_file->data(), ruleset_file->length());
  if (!flat::VerifyExtensionIndexedRulesetBuffer(verifier)) {
    NOTREACHED() << "Buffer could not be verified";
  }

  UMA_HISTOGRAM_TIMES("Extensions.DeclarativeNetRequest.CreateVerifiedMatcher",
                      base::TimeTicks::Now() - start);
  // Using WrapUnique instead of MakeUnique since this class has a private
  // constructor.
  return base::WrapUnique(new RulesetMatcher(std::move(ruleset_file)));
}

RulesetMatcher::RulesetMatcher(
    std::unique_ptr<base::MemoryMappedFile,
                    content::BrowserThread::DeleteOnFileThread> ruleset_file)
    : ruleset_file_(std::move(ruleset_file)),
      root_(flat::GetExtensionIndexedRuleset(ruleset_file_->data())),
      blacklist_matcher_(root_->blacklist_index()),
      whitelist_matcher_(root_->whitelist_index()),
      redirect_matcher_(root_->redirect_index()),
      extension_metdata_(root_->extension_metdata()) {}

}  // namespace declarative_net_request
}  // namespace extensions
