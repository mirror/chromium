// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/ruleset_manager.h"

#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/ruleset_matcher.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"

namespace extensions {
namespace declarative_net_request {
namespace {
namespace flat_rule = ::url_pattern_index::flat;

flat_rule::ElementType GetElementType(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_LAST_TYPE:
    case content::RESOURCE_TYPE_PREFETCH:
    case content::RESOURCE_TYPE_CSP_REPORT:    // TODO
    case content::RESOURCE_TYPE_SUB_RESOURCE:  // TODO
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
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:  // TODO
      return flat_rule::ElementType_OBJECT;
    case content::RESOURCE_TYPE_XHR:
      return flat_rule::ElementType_XMLHTTPREQUEST;
    case content::RESOURCE_TYPE_MAIN_FRAME:  // TODO
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

flat_rule::ElementType GetElementType(const net::URLRequest* request) {
  DCHECK(request);
  if (request->url().SchemeIsWSOrWSS())
    return flat_rule::ElementType_WEBSOCKET;
  const auto* info = content::ResourceRequestInfo::ForRequest(request);
  return info ? GetElementType(info->GetResourceType())
              : flat_rule::ElementType_OTHER;
}

// TODO this can be shared with subresource filter.
bool IsThirdPartyRequest(const GURL& url,
                         const base::Optional<url::Origin>& document_origin) {
  if (!document_origin || document_origin->unique())
    return true;
  return !net::registry_controlled_domains::SameDomainOrHost(
      url, *document_origin,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

}  // namespace
RulesetManager::RulesMapKey::RulesMapKey(ExtensionId extension_id,
                                         base::Time extension_install_time)
    : id(std::move(extension_id)),
      install_time(std::move(extension_install_time)) {}
RulesetManager::RulesMapKey::~RulesMapKey() = default;
RulesetManager::RulesMapKey::RulesMapKey(RulesetManager::RulesMapKey&&) =
    default;

// Comparator to ensure that RulesMapKey are sorted in order of decreasing
// extension install time i.e. most recently installed extension first. For
// equal install times, sort by id. This is also necessary since a set uses the
// this to prevent duplicates.
bool RulesetManager::RulesMapKeyComparator::operator()(
    const RulesMapKey& lhs,
    const RulesMapKey& rhs) const {
  return lhs.install_time > rhs.install_time || lhs.id < rhs.id;
}

RulesetManager::RulesetManager(InfoMap* info_map) : info_map_(info_map) {
  DCHECK(info_map_);
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

RulesetManager::~RulesetManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void RulesetManager::AddRuleset(
    const ExtensionId& extension_id,
    const base::Time& install_time,
    std::unique_ptr<RulesetMatcher> ruleset_matcher) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (base::ContainsKey(install_time_map_, extension_id)) {
    return;
  }

  RulesMapKey key(extension_id, install_time);
  DCHECK(!base::ContainsKey(rules_, key));

  install_time_map_[extension_id] = install_time;
  rules_[std::move(key)] = std::move(ruleset_matcher);
}

void RulesetManager::RemoveRuleset(const ExtensionId& extension_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::ContainsKey(install_time_map_, extension_id)) {
    return;
  }

  base::Time install_time = install_time_map_[extension_id];
  const RulesMapKey key(extension_id, install_time);
  DCHECK(base::ContainsKey(rules_, key));

  rules_.erase(key);
  install_time_map_.erase(extension_id);
}

bool RulesetManager::ShouldBlockRequest(bool is_incognito_context,
                                        net::URLRequest* request) const {
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.ShouldBlockRequestDuration");
  const GURL& url = request->url();
  const base::Optional<url::Origin> initiator = request->initiator();
  const url::Origin first_party_origin =
      initiator ? initiator.value() : url::Origin();
  flat_rule::ElementType element_type = GetElementType(request);
  const bool is_third_party = IsThirdPartyRequest(url, request->initiator());

  for (const auto& pair : rules_) {
    // No special handling is needed for split mode extensions, since both
    // normal and incognito contexts share the same ruleset.
    const bool evaluate_ruleset =
        !is_incognito_context || info_map_->IsIncognitoEnabled(pair.first.id);
    if (evaluate_ruleset &&
        pair.second->ShouldBlockRequest(url, first_party_origin, element_type,
                                        is_third_party))
      return true;
  }
  return false;
}

bool RulesetManager::ShouldRedirectRequest(bool is_incognito_context,
                                           net::URLRequest* request,
                                           GURL* redirect_url) const {
  DCHECK(redirect_url);
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.ShouldRedirectRequestDuration");
  const GURL& url = request->url();
  const base::Optional<url::Origin> initiator = request->initiator();
  const url::Origin first_party_origin =
      initiator ? initiator.value() : url::Origin();
  flat_rule::ElementType element_type = GetElementType(request);
  const bool is_third_party = IsThirdPartyRequest(url, request->initiator());

  for (const auto& pair : rules_) {
    // No special handling is needed for split mode extensions, since both
    // normal and incognito contexts share the same ruleset.
    const bool evaluate_ruleset =
        !is_incognito_context || info_map_->IsIncognitoEnabled(pair.first.id);
    if (evaluate_ruleset && pair.second->ShouldRedirectRequest(
                                url, first_party_origin, element_type,
                                is_third_party, redirect_url))
      return true;
  }
  return false;
}

}  // namespace declarative_net_request
}  // namespace extensions
