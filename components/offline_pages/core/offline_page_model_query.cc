// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_model_query.h"

#include <algorithm>
#include <tuple>
#include <unordered_set>

#include "base/memory/ptr_util.h"

namespace offline_pages {

namespace {

int CountMatchingUrls(const GURL& url_pattern,
                      const std::set<GURL>& urls,
                      bool defrag) {
  int count = 0;

  // If |defrag| is true, all urls will be compared after fragments stripped.
  // Otherwise just do exact matching.
  if (defrag) {
    for (const auto& url : urls) {
      GURL::Replacements remove_params;
      remove_params.ClearRef();
      GURL url_without_fragment = url.ReplaceComponents(remove_params);
      if (url_without_fragment == url_pattern.ReplaceComponents(remove_params))
        count++;
    }
  } else {
    count = urls.count(url_pattern);
  }
  return count;
}

}  // namespace

using Requirement = OfflinePageModelQuery::Requirement;

OfflinePageModelQueryBuilder::OfflinePageModelQueryBuilder()
    : offline_ids_(std::make_pair(Requirement::UNSET, std::vector<int64_t>())) {
}

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
    const std::vector<GURL>& urls,
    URLSearchMode search_mode,
    bool defrag) {
  urls_ = std::make_tuple(requirement, urls, search_mode, defrag);
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

std::unique_ptr<OfflinePageModelQuery> OfflinePageModelQueryBuilder::Build(
    ClientPolicyController* controller) {
  DCHECK(controller);

  auto query = base::MakeUnique<OfflinePageModelQuery>();

  query->urls_ = std::make_tuple(
      std::get<0>(urls_),
      std::set<GURL>(std::get<1>(urls_).begin(), std::get<1>(urls_).end()),
      std::get<2>(urls_), std::get<3>(urls_));
  urls_ = std::make_tuple(Requirement::UNSET, std::vector<GURL>(),
                          URLSearchMode::SEARCH_BY_FINAL_URL_ONLY, false);
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

std::tuple<Requirement, std::set<GURL>, URLSearchMode, bool>
OfflinePageModelQuery::GetRestrictedToUrls() const {
  if (std::get<0>(urls_) == Requirement::UNSET)
    return std::make_tuple(Requirement::UNSET, std::set<GURL>(),
                           URLSearchMode::SEARCH_BY_FINAL_URL_ONLY, false);
  return urls_;
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

  Requirement url_requirement = std::get<0>(urls_);
  const std::set<GURL> url_set = std::get<1>(urls_);
  URLSearchMode search_mode = std::get<2>(urls_);
  bool defrag = std::get<3>(urls_);
  if (url_requirement != Requirement::UNSET) {
    int count = CountMatchingUrls(item.url, url_set, defrag);
    if (search_mode == URLSearchMode::SEARCH_BY_FINAL_URL_ONLY) {
      if ((url_requirement == Requirement::INCLUDE_MATCHING && count == 0) ||
          (url_requirement == Requirement::EXCLUDE_MATCHING && count > 0)) {
        return false;
      }
    } else {
      if ((url_requirement == Requirement::INCLUDE_MATCHING && count == 0 &&
           url_set.count(item.original_url) == 0) ||
          (url_requirement == Requirement::EXCLUDE_MATCHING &&
           (count > 0 || url_set.count(item.original_url) > 0)))
        return false;
    }
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
