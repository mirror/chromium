// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/generate_page_bundle_reconciler.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {

// Temporary storage for Urls metadata fetched from the storage.
struct FetchedUrl {
  FetchedUrl() = default;
  FetchedUrl(int64_t offline_id, const std::string& requested_url)
      : offline_id(offline_id), requested_url(requested_url) {}

  int64_t offline_id;
  std::string requested_url;
};

std::unique_ptr<std::vector<FetchedUrl>> GetAllUrlsMarkedRequestedSync(
    sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id, requested_url"
      " FROM prefetch_items"
      " WHERE state = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(
      0, static_cast<int>(PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE));

  auto urls = base::MakeUnique<std::vector<FetchedUrl>>();
  while (statement.Step()) {
    urls->emplace_back(
        FetchedUrl(statement.ColumnInt64(0), statement.ColumnString(1)));
  }
  return urls;
}

bool UpdateUrlSync(const int64_t offline_id,
                   int max_attempts,
                   sql::Connection* db) {
  // Transit to NEW_REQUEST state if not exceeding maximum attempts
  // Transit to FINISHED state with error_code set otherwise.
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = CASE WHEN generate_bundle_attempts < ? THEN ?"
      "                  ELSE ? END,"
      "     error_code = CASE WHEN generate_bundle_attempts < ? THEN error_code"
      "                       ELSE ? END"
      " WHERE offline_id = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, max_attempts);
  statement.BindInt(1, static_cast<int>(PrefetchItemState::NEW_REQUEST));
  statement.BindInt(2, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(3, max_attempts);
  statement.BindInt(
      4,
      static_cast<int>(PrefetchItemErrorCode::
                           GENERATE_PAGE_BUNDLE_REQUEST_MAX_ATTEMPTS_REACHED));
  statement.BindInt64(5, offline_id);

  return statement.Run();
}

bool ReconcileGenerateBundleRequests(
    std::unique_ptr<std::set<std::string>> requested_urls,
    int max_attempts,
    sql::Connection* db) {
  if (!db || !requested_urls)
    return false;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  auto urls = GetAllUrlsMarkedRequestedSync(db);
  DCHECK(urls);
  if (urls->empty())
    return false;

  for (const auto& url : *urls) {
    // If the url is a part of active request, do nothing.
    if (requested_urls->find(url.requested_url) != requested_urls->end())
      continue;

    // Otherwise, update the state to either retry request or error out.
    if (!UpdateUrlSync(url.offline_id, max_attempts, db))
      return false;
  }

  return transaction.Commit();
}
}  // namespace

// static
const int GeneratePageBundleReconciler::kMaxGenerateBundleAttempts = 3;

GeneratePageBundleReconciler::GeneratePageBundleReconciler(
    PrefetchStore* prefetch_store,
    PrefetchNetworkRequestFactory* request_factory)
    : prefetch_store_(prefetch_store),
      request_factory_(request_factory),
      weak_factory_(this) {}

GeneratePageBundleReconciler::~GeneratePageBundleReconciler() {}

void GeneratePageBundleReconciler::Run() {
  std::unique_ptr<std::set<std::string>> requested_urls =
      request_factory_->GetAllUrlsRequested();
  prefetch_store_->Execute(
      base::BindOnce(&ReconcileGenerateBundleRequests,
                     std::move(requested_urls), kMaxGenerateBundleAttempts),
      base::BindOnce(&GeneratePageBundleReconciler::FinishedUpdate,
                     weak_factory_.GetWeakPtr()));
}

void GeneratePageBundleReconciler::FinishedUpdate(bool success) {
  // TODO(dimich): report failure/success to UMA.
  TaskComplete();
}

}  // namespace offline_pages