// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/ruleset_manager.h"

#include <utility>

#include "base/stl_util.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/ruleset_matcher.h"
#include "extensions/common/api/declarative_net_request/utils.h"
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
    // TODO(karandeepb): Add support for CSP_REPORT to url_pattern_index. This
    // is supported by the Web Request API.
    case content::RESOURCE_TYPE_CSP_REPORT:
    case content::RESOURCE_TYPE_SUB_RESOURCE:
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
    // COMMENT: Both subresource filter and Web Request API treat
    // PLUGIN_RESOURCE as OBJECT instead of OBJECT_SUBREQUEST. Why?
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
      return flat_rule::ElementType_OBJECT;
    case content::RESOURCE_TYPE_XHR:
      return flat_rule::ElementType_XMLHTTPREQUEST;
    // TODO(karandeepb): Add support for MAIN_FRAME to url_pattern_index. This
    // is supported by the Web Request API.
    case content::RESOURCE_TYPE_MAIN_FRAME:
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

// Returns whether the request to |url| is third party to its |document_origin|.
// TODO(karandeepb): Look into caching this.
bool IsThirdPartyRequest(const GURL& url, const url::Origin& document_origin) {
  if (document_origin.unique())
    return true;

  return !net::registry_controlled_domains::SameDomainOrHost(
      url, document_origin,
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
  // COMMENT: I can rely on the fact that OnExtensionLoaded is not called twice
  // in succession, right?
  DCHECK(!base::ContainsKey(install_time_map_, extension_id));
  DCHECK(IsAPIAvailable());

  RulesMapKey key(extension_id, install_time);
  DCHECK(!base::ContainsKey(rules_, key));

  install_time_map_[extension_id] = install_time;
  rules_[std::move(key)] = std::move(ruleset_matcher);
}

void RulesetManager::RemoveRuleset(const ExtensionId& extension_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsAPIAvailable());

  auto it = install_time_map_.find(extension_id);
  // This may happen when loading the ruleset for an extension failed.
  if (it == install_time_map_.end())
    return;

  const RulesMapKey key(extension_id, it->second);
  DCHECK(base::ContainsKey(rules_, key));

  install_time_map_.erase(it);
  rules_.erase(key);
}

bool RulesetManager::ShouldBlockRequest(bool is_incognito_context,
                                        net::URLRequest* request) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(request);

  const GURL& url = request->url();
  const url::Origin first_party_origin =
      request->initiator() ? request->initiator().value() : url::Origin();
  const flat_rule::ElementType element_type = GetElementType(request);
  const bool is_third_party = IsThirdPartyRequest(url, first_party_origin);

  for (const auto& pair : rules_) {
    // No special handling is needed for split mode incognito extensions, since
    // both normal and incognito contexts share the same ruleset.
    const bool evaluate_ruleset =
        !is_incognito_context || info_map_->IsIncognitoEnabled(pair.first.id);

    // TODO(karandeepb): Check host permissions etc. For e.g. an extension
    // shouldn't be able to block an internal page.
    if (evaluate_ruleset &&
        pair.second->ShouldBlockRequest(url, first_party_origin, element_type,
                                        is_third_party)) {
      return true;
    }
  }
  return false;
}

bool RulesetManager::ShouldRedirectRequest(bool is_incognito_context,
                                           net::URLRequest* request,
                                           GURL* redirect_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(request);
  DCHECK(redirect_url);

  const GURL& url = request->url();
  const url::Origin first_party_origin =
      request->initiator() ? request->initiator().value() : url::Origin();
  const flat_rule::ElementType element_type = GetElementType(request);
  const bool is_third_party = IsThirdPartyRequest(url, first_party_origin);

  for (const auto& pair : rules_) {
    // No special handling is needed for split mode extensions, since both
    // normal and incognito contexts share the same ruleset.
    const bool evaluate_ruleset =
        !is_incognito_context || info_map_->IsIncognitoEnabled(pair.first.id);

    // TODO(karandeepb): Check host permissions etc. For e.g. an extension
    // shouldn't be able to redirect an internal page.
    if (evaluate_ruleset && pair.second->ShouldRedirectRequest(
                                url, first_party_origin, element_type,
                                is_third_party, redirect_url)) {
      return true;
    }
  }
  return false;
}

}  // namespace declarative_net_request
}  // namespace extensions
