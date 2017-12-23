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
#include "url/gurl.h"

namespace extensions {
namespace declarative_net_request {
namespace flat_rule = url_pattern_index::flat;

namespace {

void DeleteRulesetHelper(std::unique_ptr<base::MemoryMappedFile> ruleset) {
  base::AssertBlockingAllowed();
}

using FindRuleStrategy =
    url_pattern_index::UrlPatternIndexMatcher::FindRuleStrategy;

class MemoryMappedDataWrapper : public RulesetMatcher::RulesetDataWrapper {
 public:
  MemoryMappedDataWrapper(const base::FilePath& file_path) {
    file_ = std::make_unique<base::MemoryMappedFile>();
    CHECK(file_->Initialize(file_path, base::MemoryMappedFile::READ_ONLY));
  }

  ~MemoryMappedDataWrapper() override {
    base::PostTaskWithTraits(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::BindOnce(&DeleteRulesetHelper, std::move(file_)));
  }

  const uint8_t* data() const override { return file_->data(); }

  size_t length() const override { return file_->length(); }

 private:
  std::unique_ptr<base::MemoryMappedFile> file_;

  DISALLOW_COPY_AND_ASSIGN(MemoryMappedDataWrapper);
};

class InMemoryDataWrapper : public RulesetMatcher::RulesetDataWrapper {
 public:
  InMemoryDataWrapper(const base::FilePath& file_path) {
    CHECK(base::ReadFileToString(file_path, &data_));
  }

  ~InMemoryDataWrapper() override = default;

  const uint8_t* data() const override {
    return reinterpret_cast<const uint8_t*>(data_.data());
  }

  size_t length() const override { return data_.size(); }

 private:
  std::string data_;
  DISALLOW_COPY_AND_ASSIGN(InMemoryDataWrapper);
};

std::unique_ptr<RulesetMatcher::RulesetDataWrapper> CreateDataWrapper(
    const base::FilePath& file_path,
    RulesetMatcher::RulesetDataPolicy policy) {
  switch (policy) {
    case RulesetMatcher::RulesetDataPolicy::kMemoryMap:
      return std::make_unique<MemoryMappedDataWrapper>(file_path);
      break;
    case RulesetMatcher::RulesetDataPolicy::kInMemory:
      return std::make_unique<InMemoryDataWrapper>(file_path);
      break;
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace

// static
RulesetMatcher::LoadRulesetResult RulesetMatcher::CreateVerifiedMatcher(
    const base::FilePath& indexed_ruleset_path,
    int expected_ruleset_checksum,
    std::unique_ptr<RulesetMatcher>* matcher) {
  DCHECK(matcher);
  DCHECK(IsAPIAvailable());
  base::AssertBlockingAllowed();

  base::ElapsedTimer timer;

  if (!base::PathExists(indexed_ruleset_path))
    return kLoadErrorInvalidPath;

  auto ruleset = CreateDataWrapper(indexed_ruleset_path, default_policy);

  // This guarantees that no memory access will end up outside the buffer.
  if (!IsValidRulesetData(ruleset->data(), ruleset->length(),
                          expected_ruleset_checksum)) {
    return kLoadErrorRulesetVerification;
  }

  UMA_HISTOGRAM_TIMES(
      "Extensions.DeclarativeNetRequest.CreateVerifiedMatcherTime",
      timer.Elapsed());

  // Using WrapUnique instead of make_unique since this class has a private
  // constructor.
  *matcher = base::WrapUnique(new RulesetMatcher(std::move(ruleset)));
  return kLoadSuccess;
}

RulesetMatcher::~RulesetMatcher() = default;

bool RulesetMatcher::ShouldBlockRequest(const GURL& url,
                                        const url::Origin& first_party_origin,
                                        flat_rule::ElementType element_type,
                                        bool is_third_party) const {
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.ShouldBlockRequestTime."
      "SingleExtension");

  // Don't exclude generic rules from being matched. A generic rule is one with
  // an empty included domains list.
  const bool disable_generic_rules = false;

  const url_pattern_index::flat::UrlRule* blacklist =
      blacklist_matcher_.FindMatch(
          url, first_party_origin, element_type, flat_rule::ActivationType_NONE,
          is_third_party, disable_generic_rules, FindRuleStrategy::kAny);

  const url_pattern_index::flat::UrlRule* whitelist = whitelist_matcher_.FindMatch(
          url, first_party_origin, element_type, flat_rule::ActivationType_NONE,
          is_third_party, disable_generic_rules, FindRuleStrategy::kAny);


  const std::string search_url = "yimg.com/rq/darla/";
  if (url.spec().find(search_url) != std::string::npos) {
    LOG(ERROR) << "--------search_url " << search_url << "\n";
    LOG(ERROR) << "--------url.spec() " << url.spec() << "\n";
    if (blacklist)
      LOG(ERROR) << "--------blacklist->id() " << blacklist->id() << "\n";
    if (whitelist)
      LOG(ERROR) << "--------whitelist->id() " << whitelist->id() << "\n";
  }
  return blacklist && !whitelist;
}

bool RulesetMatcher::ShouldRedirectRequest(
    const GURL& url,
    const url::Origin& first_party_origin,
    flat_rule::ElementType element_type,
    bool is_third_party,
    GURL* redirect_url) const {
  DCHECK(redirect_url);
  DCHECK_NE(flat_rule::ElementType_WEBSOCKET, element_type);

  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.ShouldRedirectRequestTime."
      "SingleExtension");

  // Don't exclude generic rules from being matched. A generic rule is one with
  // an empty included domains list.
  const bool disable_generic_rules = false;

  // Retrieve the highest priority matching rule corresponding to the given
  // request parameters.
  const flat_rule::UrlRule* rule = redirect_matcher_.FindMatch(
      url, first_party_origin, element_type, flat_rule::ActivationType_NONE,
      is_third_party, disable_generic_rules,
      FindRuleStrategy::kHighestPriority);
  if (!rule)
    return false;

  // Find the UrlRuleMetadata corresponding to |rule|. Since |metadata_list_| is
  // sorted by rule id, use LookupByKey which binary searches for fast lookup.
  const flat::UrlRuleMetadata* metadata =
      metadata_list_->LookupByKey(rule->id());

  // There must be a UrlRuleMetadata object corresponding to each redirect rule.
  DCHECK(metadata);
  DCHECK_EQ(metadata->id(), rule->id());

  *redirect_url = GURL(base::StringPiece(metadata->redirect_url()->c_str(),
                                         metadata->redirect_url()->size()));
  DCHECK(redirect_url->is_valid());
  return true;
}

RulesetMatcher::RulesetMatcher(std::unique_ptr<RulesetDataWrapper> ruleset)
    : ruleset_(std::move(ruleset)),
      root_(flat::GetExtensionIndexedRuleset(ruleset_->data())),
      blacklist_matcher_(root_->blacklist_index()),
      whitelist_matcher_(root_->whitelist_index()),
      redirect_matcher_(root_->redirect_index()),
      metadata_list_(root_->extension_metadata()) {}

RulesetMatcher::RulesetDataPolicy RulesetMatcher::default_policy =
    RulesetMatcher::RulesetDataPolicy::kInMemory;

}  // namespace declarative_net_request
}  // namespace extensions
