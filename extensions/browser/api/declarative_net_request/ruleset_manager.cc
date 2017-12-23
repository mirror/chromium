// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"

#include <algorithm>
#include <tuple>
#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/timer/elapsed_timer.h"
#include "base/stl_util.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "components/web_cache/browser/web_cache_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace flat_rule = url_pattern_index::flat;

// Maps content::ResourceType to flat_rule::ElementType.
flat_rule::ElementType GetElementType(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_LAST_TYPE:
    case content::RESOURCE_TYPE_PREFETCH:
    case content::RESOURCE_TYPE_SUB_RESOURCE:
    // TODO(crbug.com/696822): Add support for main frame and csp report to
    // url_pattern_index. These are supported by the Web Request API.
    case content::RESOURCE_TYPE_MAIN_FRAME:
    case content::RESOURCE_TYPE_CSP_REPORT:
      return flat_rule::ElementType_OTHER;
    case content::RESOURCE_TYPE_SCRIPT:
    case content::RESOURCE_TYPE_WORKER:
    case content::RESOURCE_TYPE_SHARED_WORKER:
    case content::RESOURCE_TYPE_SERVICE_WORKER:
      return flat_rule::ElementType_SCRIPT;
    case content::RESOURCE_TYPE_IMAGE:
    case content::RESOURCE_TYPE_FAVICON:
      return flat_rule::ElementType_IMAGE;
    case content::RESOURCE_TYPE_STYLESHEET:
      return flat_rule::ElementType_STYLESHEET;
    case content::RESOURCE_TYPE_OBJECT:
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
      return flat_rule::ElementType_OBJECT;
    case content::RESOURCE_TYPE_XHR:
      return flat_rule::ElementType_XMLHTTPREQUEST;
    case content::RESOURCE_TYPE_SUB_FRAME:
      return flat_rule::ElementType_SUBDOCUMENT;
    case content::RESOURCE_TYPE_PING:
      return flat_rule::ElementType_PING;
    case content::RESOURCE_TYPE_MEDIA:
      return flat_rule::ElementType_MEDIA;
    case content::RESOURCE_TYPE_FONT_RESOURCE:
      return flat_rule::ElementType_FONT;
  }
  NOTREACHED();
  return flat_rule::ElementType_OTHER;
}

// Returns the flat_rule::ElementType for the given |request|.
flat_rule::ElementType GetElementType(const net::URLRequest& request) {
  if (request.url().SchemeIsWSOrWSS())
    return flat_rule::ElementType_WEBSOCKET;

  const auto* info = content::ResourceRequestInfo::ForRequest(&request);
  return info ? GetElementType(info->GetResourceType())
              : flat_rule::ElementType_OTHER;
}

// Returns whether the request to |url| is third party to its
// |document_origin|.
// TODO(crbug.com/696822): Look into caching this.
bool IsThirdPartyRequest(const GURL& url, const url::Origin& document_origin) {
  if (document_origin.unique())
    return true;

  return !net::registry_controlled_domains::SameDomainOrHost(
      url, document_origin,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

void ClearRendererCacheOnUI() {
  web_cache::WebCacheManager::GetInstance()->ClearCacheOnNavigation();
}

// Helper to clear each renderer's in-memory cache the next time it
// navigates.
void ClearRendererCacheOnNavigation() {
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    ClearRendererCacheOnUI();
  } else {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::BindOnce(&ClearRendererCacheOnUI));
  }
}

}  // namespace

class RulesetManager::Core {
 public:
  ~Core();
  Core();

  void AddRuleset(RulesetManager::ExtensionRulesetData);
  void RemoveRuleset(const ExtensionId& extension_id);
  RulesetManager::Result EvaluateRuleset(bool is_incognito_context,
                                         const GURL& url,
                                         const url::Origin& first_party_origin,
                                         flat_rule::ElementType element_type,
                                         bool is_third_party) const {
    base::ElapsedTimer timer;
    DCHECK_CALLED_ON_VALID_SEQUENCE(core_sequence_checker_);

    Result result;
    if (ShouldBlockRequest(url, first_party_origin, element_type,
                           is_third_party, is_incognito_context, &result)) {
    } else if (ShouldRedirectRequest(url, first_party_origin, element_type,
                                     is_third_party, is_incognito_context,
                                     &result)) {
    }
    ruleset_eval_time_ += timer.Elapsed();
    max_eval_time_ = std::max(max_eval_time_, timer.Elapsed());
    return result;
  }
  void PrintRulesetEvalTime() {
    LOG(ERROR) << "--------ruleset_eval_time_ " << ruleset_eval_time_.InMillisecondsF() << "\n";
    LOG(ERROR) << "--------max_eval_time_ " << max_eval_time_.InMillisecondsF() << "\n";
  }

 private:
  friend class RulesetManager;

  mutable base::TimeDelta ruleset_eval_time_;
  mutable base::TimeDelta max_eval_time_;

  bool ShouldBlockRequest(const GURL& url,
                          const url::Origin& first_party_origin,
                          flat_rule::ElementType element_type,
                          bool is_third_party,
                          bool is_incognito_context,
                          RulesetManager::Result* result) const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(core_sequence_checker_);
    DCHECK(result);

    // Return early if DNR is not enabled.
    if (!IsAPIAvailable())
      return false;

    SCOPED_UMA_HISTOGRAM_TIMER(
        "Extensions.DeclarativeNetRequest.ShouldBlockRequestTime."
        "AllExtensions");

    for (const auto& ruleset_data : rulesets_) {
      const bool evaluate_ruleset =
          !is_incognito_context || ruleset_data.is_incognito_enabled;

      // TODO(crbug.com/777714): Check host permissions etc.
      if (evaluate_ruleset &&
          ruleset_data.matcher->ShouldBlockRequest(
              url, first_party_origin, element_type, is_third_party)) {
        result->extension_id = ruleset_data.extension_id;
        result->extension_install_time = ruleset_data.extension_install_time;
        result->cancel = true;
        return true;
      }
    }
    return false;
  }
  bool ShouldRedirectRequest(const GURL& url,
                             const url::Origin& first_party_origin,
                             flat_rule::ElementType element_type,
                             bool is_third_party,
                             bool is_incognito_context,
                             RulesetManager::Result* result) const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(core_sequence_checker_);
    DCHECK(result);

    // Return early if DNR is not enabled.
    if (!IsAPIAvailable())
      return false;

    // Redirecting WebSocket handshake request is prohibited.
    if (element_type == flat_rule::ElementType_WEBSOCKET)
      return false;

    SCOPED_UMA_HISTOGRAM_TIMER(
        "Extensions.DeclarativeNetRequest.ShouldRedirectRequestTime."
        "AllExtensions");

    // This iterates in decreasing order of extension installation time. Hence
    // more recently installed extensions get higher priority in choosing the
    // redirect url.
    for (const auto& ruleset_data : rulesets_) {
      const bool evaluate_ruleset =
          !is_incognito_context || ruleset_data.is_incognito_enabled;

      // TODO(crbug.com/777714): Check host permissions etc.
      GURL redirect_url;
      if (evaluate_ruleset && ruleset_data.matcher->ShouldRedirectRequest(
                                  url, first_party_origin, element_type,
                                  is_third_party, &redirect_url)) {
        result->extension_id = ruleset_data.extension_id;
        result->extension_install_time = ruleset_data.extension_install_time;
        *(result->redirect_url) = std::move(redirect_url);
        return true;
      }
    }

    return false;
  }

  // Sorted in decreasing order of |extension_install_time|.
  // Use a flat_set instead of std::set/map. This makes [Add/Remove]Ruleset
  // O(n), but it's fine since the no. of rulesets are expected to be quite
  // small.
  base::flat_set<RulesetManager::ExtensionRulesetData> rulesets_;

  SEQUENCE_CHECKER(core_sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(Core);
};

RulesetManager::RulesetManager(const InfoMap* info_map)
    : info_map_(info_map),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      core_(new Core()),
      core_async_(new Core(), base::OnTaskRunnerDeleter(file_task_runner_)) {
  DCHECK(info_map_);

  // RulesetManager can be created on any sequence.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

RulesetManager::~RulesetManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void RulesetManager::AddRuleset(
    const ExtensionId& extension_id,
    std::unique_ptr<RulesetMatcher> ruleset_matcher) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsAPIAvailable());

  has_added_ruleset_ = true;

  ExtensionRulesetData data(
      extension_id, info_map_->GetInstallTime(extension_id),
      info_map_->IsIncognitoEnabled(extension_id), std::move(ruleset_matcher));
  base::OnceClosure task = base::BindOnce(
      &Core::AddRuleset, base::Unretained(GetCore()), std::move(data));

  if (async_)
    file_task_runner_->PostTask(FROM_HERE, std::move(task));
  else
    std::move(task).Run();
}

void RulesetManager::SetAsync() {
  DCHECK(core_->rulesets_.empty());
  async_ = true;
}

void RulesetManager::Core::AddRuleset(ExtensionRulesetData data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(core_sequence_checker_);

  rulesets_.insert(std::move(data));

  // Clear the renderers' cache so that they take the new rules into
  // account.
  ClearRendererCacheOnNavigation();
}

void RulesetManager::RemoveRuleset(const ExtensionId& extension_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsAPIAvailable());

  base::OnceClosure task = base::BindOnce(
      &Core::RemoveRuleset, base::Unretained(GetCore()), extension_id);
  if (async_) {
    file_task_runner_->PostTask(FROM_HERE, std::move(task));
  } else {
    std::move(task).Run();
  }
}

void RulesetManager::Core::RemoveRuleset(const ExtensionId& extension_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(core_sequence_checker_);

  auto compare_by_id =
      [&extension_id](const ExtensionRulesetData& ruleset_data) {
        return ruleset_data.extension_id == extension_id;
      };

  // DCHECK(std::find_if(rulesets_.begin(), rulesets_.end(), compare_by_id)
  // !=
  //        rulesets_.end())
  //     << "RemoveRuleset called without a corresponding AddRuleset for "
  //     << extension_id;

  base::EraseIf(rulesets_, compare_by_id);

  // Clear the renderers' cache so that they take the removed rules into
  // account.
  ClearRendererCacheOnNavigation();
}

base::Optional<RulesetManager::Result> RulesetManager::EvaluateRuleset(
    const net::URLRequest& request,
    bool is_incognito_context,
    EvaluateRulesetCallback callback) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (test_observer_)
    test_observer_->OnEvaluateRuleset(request, is_incognito_context);

  if (!has_added_ruleset_)
    return RulesetManager::Result();

  GURL url = request.url();
  url::Origin first_party_origin = request.initiator().value_or(url::Origin());
  const flat_rule::ElementType element_type = GetElementType(request);

  base::ElapsedTimer timer;
  const bool is_third_party = IsThirdPartyRequest(url, first_party_origin);
  third_party_time_ += timer.Elapsed();

  base::OnceCallback<Result()> task = base::BindOnce(
      &RulesetManager::Core::EvaluateRuleset, base::Unretained(GetCore()),
      is_incognito_context, std::move(url), std::move(first_party_origin),
      element_type, is_third_party);
  if (async_) {
    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
                                     std::move(task), std::move(callback));
    return base::nullopt;
  } else {
    return std::move(task).Run();
  }
}

void RulesetManager::SetObserverForTest(TestObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!async_);
  test_observer_ = observer;
}

RulesetManager::ExtensionRulesetData::ExtensionRulesetData(
    ExtensionId extension_id,
    base::Time extension_install_time,
    bool is_incognito_enabled,
    std::unique_ptr<RulesetMatcher> matcher)
    : extension_id(std::move(extension_id)),
      extension_install_time(std::move(extension_install_time)),
      is_incognito_enabled(is_incognito_enabled),
      matcher(std::move(matcher)) {}
RulesetManager::ExtensionRulesetData::~ExtensionRulesetData() = default;
RulesetManager::ExtensionRulesetData::ExtensionRulesetData(
    ExtensionRulesetData&& other) = default;
RulesetManager::ExtensionRulesetData& RulesetManager::ExtensionRulesetData::
operator=(ExtensionRulesetData&& other) = default;

bool RulesetManager::ExtensionRulesetData::operator<(
    const ExtensionRulesetData& other) const {
  // Sort based on descending installation time, using extension id to break
  // ties.
  return (extension_install_time != other.extension_install_time)
             ? (extension_install_time > other.extension_install_time)
             : (extension_id < other.extension_id);
}

RulesetManager::Core* RulesetManager::GetCore() const {
  if (async_)
    return core_async_.get();
  return core_.get();
}

RulesetManager::Result::Result() = default;
RulesetManager::Result::~Result() = default;
RulesetManager::Result::Result(const Result&) = default;
RulesetManager::Core::Core() {
  DETACH_FROM_SEQUENCE(core_sequence_checker_);
}

void RulesetManager::PrintRulesetEvalTime() {
  LOG(ERROR) << "--------third_party_time_ " << third_party_time_.InMillisecondsF() << "\n";
  base::OnceClosure task = base::BindOnce(&Core::PrintRulesetEvalTime, base::Unretained(GetCore()));
  if (async_)
    file_task_runner_->PostTask(FROM_HERE, std::move(task));
  else
    std::move(task).Run();
}

RulesetManager::Core::~Core() = default;

size_t RulesetManager::GetMatcherCountForTest() const {
  return core_->rulesets_.size();
}

}  // namespace declarative_net_request
}  // namespace extensions
