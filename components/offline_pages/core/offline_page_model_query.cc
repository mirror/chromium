// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_model_query.h"

#include <algorithm>
#include <unordered_set>

#include "base/memory/ptr_util.h"

namespace offline_pages {

using Requirement = OfflinePageModelQuery::Requirement;

OfflinePageModelQueryBuilder::OfflinePageModelQueryBuilder()
    : offline_ids_(std::make_pair(Requirement::UNSET, std::vector<int64_t>())),
      search_by_final_url_only_(false) {}

OfflinePageModelQueryBuilder::~OfflinePageModelQueryBuilder() = default;

OfflinePageModelQueryBuilder& OfflinePageModelQueryBuilder::SetOfflinePageIds(
    Requirement requirement,
    const std::vector<int64_t>& ids) {
  offline_ids_ = std::make_pair(requirement, ids);
  return *this;
}

OfflinePageModelQueryBuilder& OfflinePageModelQueryBuilder::SetClientIds(
    Requirement requirement,
    const std::vector<ClientId>& ids) {
  client_ids_ = std::make_pair(requirement, ids);
  return *this;
}

OfflinePageModelQueryBuilder& OfflinePageModelQueryBuilder::SetUrls(
    Requirement requirement,
    const std::vector<GURL>& urls) {
  urls_ = std::make_pair(requirement, urls);
  return *this;
}

OfflinePageModelQueryBuilder&
OfflinePageModelQueryBuilder::RequireRemovedOnCacheReset(
    Requirement removed_on_cache_reset) {
  removed_on_cache_reset_ = removed_on_cache_reset;
  return *this;
}

OfflinePageModelQueryBuilder&
OfflinePageModelQueryBuilder::RequireSupportedByDownload(
    Requirement supported_by_download) {
  supported_by_download_ = supported_by_download;
  return *this;
}

OfflinePageModelQueryBuilder&
OfflinePageModelQueryBuilder::RequireShownAsRecentlyVisitedSite(
    Requirement recently_visited) {
  shown_as_recently_visited_site_ = recently_visited;
  return *this;
}

OfflinePageModelQueryBuilder&
OfflinePageModelQueryBuilder::RequireRestrictedToOriginalTab(
    Requirement original_tab) {
  restricted_to_original_tab_ = original_tab;
  return *this;
}

OfflinePageModelQueryBuilder& OfflinePageModelQueryBuilder::RequireNamespace(
    const std::string& name_space) {
  name_space_ = base::MakeUnique<std::string>(name_space);
  return *this;
}

OfflinePageModelQueryBuilder&
OfflinePageModelQueryBuilder::SetSearchByFinalURLOnly(
    bool search_by_final_url_only) {
  search_by_final_url_only_ = search_by_final_url_only;
  return *this;
}

std::unique_ptr<OfflinePageModelQuery> OfflinePageModelQueryBuilder::Build(
    ClientPolicyController* controller) {
  DCHECK(controller);

  auto query = base::MakeUnique<OfflinePageModelQuery>();

  query->urls_ = std::make_pair(
      urls_.first, std::set<GURL>(urls_.second.begin(), urls_.second.end()));
  query->search_by_final_url_only_ = search_by_final_url_only_;
  urls_ = std::make_pair(Requirement::UNSET, std::vector<GURL>());
  search_by_final_url_only_ = false;
  query->offline_ids_ = std::make_pair(
      offline_ids_.first, std::set<int64_t>(offline_ids_.second.begin(),
                                            offline_ids_.second.end()));
  offline_ids_ = std::make_pair(Requirement::UNSET, std::vector<int64_t>());
  query->client_ids_ = std::make_pair(
      client_ids_.first,
      std::set<ClientId>(client_ids_.second.begin(), client_ids_.second.end()));
  client_ids_ = std::make_pair(Requirement::UNSET, std::vector<ClientId>());

  std::vector<std::string> allowed_namespaces;
  bool uses_namespace_restrictions = false;

  std::vector<std::string> namespaces;
  if (name_space_) {
    uses_namespace_restrictions = true;
    namespaces.push_back(*name_space_);
  } else {
    namespaces = controller->GetAllNamespaces();
  }

  for (auto& name_space : namespaces) {
    // If any exclusion requirements exist, and the namespace matches one of
    // those excluded by policy, skip adding it to |allowed_namespaces|.
    if ((removed_on_cache_reset_ == Requirement::EXCLUDE_MATCHING &&
         controller->IsRemovedOnCacheReset(name_space)) ||
        (supported_by_download_ == Requirement::EXCLUDE_MATCHING &&
         controller->IsSupportedByDownload(name_space)) ||
        (shown_as_recently_visited_site_ == Requirement::EXCLUDE_MATCHING &&
         controller->IsShownAsRecentlyVisitedSite(name_space)) ||
        (restricted_to_original_tab_ == Requirement::EXCLUDE_MATCHING &&
         controller->IsRestrictedToOriginalTab(name_space))) {
      // Skip namespaces that meet exclusion requirements.
      uses_namespace_restrictions = true;
      continue;
    }

    if ((removed_on_cache_reset_ == Requirement::INCLUDE_MATCHING &&
         !controller->IsRemovedOnCacheReset(name_space)) ||
        (supported_by_download_ == Requirement::INCLUDE_MATCHING &&
         !controller->IsSupportedByDownload(name_space)) ||
        (shown_as_recently_visited_site_ == Requirement::INCLUDE_MATCHING &&
         !controller->IsShownAsRecentlyVisitedSite(name_space)) ||
        (restricted_to_original_tab_ == Requirement::INCLUDE_MATCHING &&
         !controller->IsRestrictedToOriginalTab(name_space))) {
      // Skip namespaces that don't meet inclusion requirement.
      uses_namespace_restrictions = true;
      continue;
    }

    // Add namespace otherwise.
    allowed_namespaces.emplace_back(name_space);
  }

  removed_on_cache_reset_ = Requirement::UNSET;
  supported_by_download_ = Requirement::UNSET;
  shown_as_recently_visited_site_ = Requirement::UNSET;
  restricted_to_original_tab_ = Requirement::UNSET;

  if (uses_namespace_restrictions) {
    query->restricted_to_namespaces_ = base::MakeUnique<std::set<std::string>>(
        allowed_namespaces.begin(), allowed_namespaces.end());
  }

  return query;
}

OfflinePageModelQuery::OfflinePageModelQuery() = default;
OfflinePageModelQuery::~OfflinePageModelQuery() = default;

std::pair<bool, std::set<std::string>>
OfflinePageModelQuery::GetRestrictedToNamespaces() const {
  if (!restricted_to_namespaces_)
    return std::make_pair(false, std::set<std::string>());

  return std::make_pair(true, *restricted_to_namespaces_);
}

std::pair<Requirement, std::set<int64_t>>
OfflinePageModelQuery::GetRestrictedToOfflineIds() const {
  if (offline_ids_.first == Requirement::UNSET)
    return std::make_pair(Requirement::UNSET, std::set<int64_t>());

  return offline_ids_;
}

std::pair<Requirement, std::set<ClientId>>
OfflinePageModelQuery::GetRestrictedToClientIds() const {
  if (client_ids_.first == Requirement::UNSET)
    return std::make_pair(Requirement::UNSET, std::set<ClientId>());

  return client_ids_;
}

std::pair<Requirement, std::set<GURL>>
OfflinePageModelQuery::GetRestrictedToUrls() const {
  if (urls_.first == Requirement::UNSET)
    return std::make_pair(Requirement::UNSET, std::set<GURL>());

  return urls_;
}

bool OfflinePageModelQuery::GetSearchByFinalURLOnly() const {
  return search_by_final_url_only_;
}

bool OfflinePageModelQuery::Matches(const OfflinePageItem& item) const {
  switch (offline_ids_.first) {
    case Requirement::UNSET:
      break;
    case Requirement::INCLUDE_MATCHING:
      if (offline_ids_.second.count(item.offline_id) == 0)
        return false;
      break;
    case Requirement::EXCLUDE_MATCHING:
      if (offline_ids_.second.count(item.offline_id) > 0)
        return false;
      break;
  }

  int count = 0;
  switch (urls_.first) {
    case Requirement::UNSET:
      break;
    case Requirement::INCLUDE_MATCHING:
      // First, search by last committed URL with fragment stripped, and record
      // the matching count.
      for (const auto& url : urls_.second) {
        GURL::Replacements remove_params;
        remove_params.ClearRef();
        GURL url_without_fragment = url.ReplaceComponents(remove_params);
        if (url_without_fragment == item.url.ReplaceComponents(remove_params))
          count++;
      }
      // Then, search by original request URL if |url_search_mode| wants it.
      // Note that we want to do the exact match with fragment included. This is
      // because original URL is used for redirect purpose and it is always
      // safer to support the exact redirect.
      // If there's no matching for either last committed URL or original url,
      // the item doesn't match with query and return false.
      if (count == 0 && (urls_.second.count(item.original_url) == 0 ||
                         search_by_final_url_only_))
        return false;
      break;
    case Requirement::EXCLUDE_MATCHING:
      if (urls_.second.count(item.url) > 0 ||
          (urls_.second.count(item.original_url) > 0 &&
           !search_by_final_url_only_))
        return false;
      break;
  }

  const ClientId& client_id = item.client_id;
  if (restricted_to_namespaces_ &&
      restricted_to_namespaces_->count(client_id.name_space) == 0) {
    return false;
  }

  switch (client_ids_.first) {
    case Requirement::UNSET:
      break;
    case Requirement::INCLUDE_MATCHING:
      if (client_ids_.second.count(client_id) == 0)
        return false;
      break;
    case Requirement::EXCLUDE_MATCHING:
      if (client_ids_.second.count(client_id) > 0)
        return false;
      break;
  }

  return true;
}

}  // namespace offline_pages
