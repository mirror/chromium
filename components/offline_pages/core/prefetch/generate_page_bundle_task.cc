// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/generate_page_bundle_task.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {
using FetchedUrl = GeneratePageBundleTask::FetchedUrl;
// This is maximum URLs that Offline Page Service can take in one request.
const int kMaxUrlsToSend = 100;

static bool UpdateStateSync(sql::Connection* db, const FetchedUrl& url) {
  static const char kSql[] =
      "UPDATE prefetch_items SET state = ?, request_archive_attempt_count = ?"
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(
      0, static_cast<int>(PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE));
  statement.BindInt(1, url.request_archive_attempt_count_ + 1);
  statement.BindInt64(2, url.offline_id_);
  return statement.Run();
}

static std::unique_ptr<std::vector<FetchedUrl>> FetchURLsSync(
    sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id, requested_url, request_archive_attempt_count"
      " FROM prefetch_items"
      " WHERE state = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::NEW_REQUEST));

  auto urls = base::MakeUnique<std::vector<FetchedUrl>>();
  while (statement.Step()) {
    urls->push_back(
        FetchedUrl(statement.ColumnInt64(0),   // offline_id
                   statement.ColumnString(1),  // requested_url
                   statement.ColumnInt(2)));   // request_archive_attempt_count
  }

  return urls;
}

static std::unique_ptr<std::vector<FetchedUrl>> SelectURLsToPrefetchSync(
    sql::Connection* db) {
  if (!db)
    return nullptr;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return nullptr;

  auto urls = FetchURLsSync(db);
  if (!urls || urls->empty())
    return nullptr;

  if (urls->size() >= kMaxUrlsToSend)
    urls->resize(kMaxUrlsToSend);

  for (const auto& url : *urls) {
    if (!UpdateStateSync(db, url))
      return nullptr;
  }

  if (!transaction.Commit())
    return nullptr;

  return urls;
}
}  // namespace

GeneratePageBundleTask::FetchedUrl::FetchedUrl() = default;

GeneratePageBundleTask::FetchedUrl::FetchedUrl(
    int64_t offline_id,
    std::string requested_url,
    int request_archive_attempt_count)
    : offline_id_(offline_id),
      requested_url_(requested_url),
      request_archive_attempt_count_(request_archive_attempt_count) {}

GeneratePageBundleTask::GeneratePageBundleTask(
    PrefetchStore* prefetch_store,
    PrefetchGCMHandler* gcm_handler,
    PrefetchNetworkRequestFactory* request_factory,
    const PrefetchRequestFinishedCallback& callback)
    : prefetch_store_(prefetch_store),
      gcm_handler_(gcm_handler),
      request_factory_(request_factory),
      callback_(callback),
      weak_factory_(this) {}

GeneratePageBundleTask::~GeneratePageBundleTask() {}

void GeneratePageBundleTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&SelectURLsToPrefetchSync),
      base::BindOnce(&GeneratePageBundleTask::StartGeneratePageBundle,
                     weak_factory_.GetWeakPtr()));
}

void GeneratePageBundleTask::StartGeneratePageBundle(
    std::unique_ptr<std::vector<FetchedUrl>> urls) {
  if (!urls || urls->empty()) {
    TaskComplete();
    return;
  }

  for (const auto& url : *urls) {
    urls_.push_back(std::move(url.requested_url_));
  }
  gcm_handler_->GetGCMToken(base::Bind(
      &GeneratePageBundleTask::GotRegistrationId, weak_factory_.GetWeakPtr()));
}

void GeneratePageBundleTask::GotRegistrationId(
    const std::string& id,
    instance_id::InstanceID::Result result) {
  request_factory_->MakeGeneratePageBundleRequest(urls_, id, callback_);
  TaskComplete();
}
}  // namespace offline_pages
