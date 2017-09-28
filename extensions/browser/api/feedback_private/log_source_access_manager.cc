// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/feedback_private/log_source_access_manager.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_split.h"
#include "base/time/default_tick_clock.h"
#include "extensions/browser/api/api_resource_manager.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/feedback_private/feedback_private_delegate.h"
#include "extensions/browser/api/feedback_private/log_source_resource.h"

namespace extensions {

namespace {

using LogSource = api::feedback_private::LogSource;
using ReadLogSourceParams = api::feedback_private::ReadLogSourceParams;
using ReadLogSourceResult = api::feedback_private::ReadLogSourceResult;
using SystemLogsResponse = system_logs::SystemLogsResponse;

const int kMaxReadersPerSource = 10;

// The minimum time between consecutive reads of a log source by a particular
// extension.
const int kDefaultRateLimitingTimeoutMs = 1000;

// If this is null, then |kDefaultRateLimitingTimeoutMs| is used as the timeout.
const base::TimeDelta* g_rate_limiting_timeout = nullptr;

base::TimeDelta GetMinTimeBetweenReads() {
  return g_rate_limiting_timeout
             ? *g_rate_limiting_timeout
             : base::TimeDelta::FromMilliseconds(kDefaultRateLimitingTimeoutMs);
}

// SystemLogsResponse is a map of strings -> strings. The map value has the
// actual log contents, a string containing all lines, separated by newlines.
// This function extracts the individual lines and converts them into a vector
// of strings, each string containing a single line.
void GetLogLinesFromSystemLogsResponse(const SystemLogsResponse& response,
                                       std::vector<std::string>* log_lines) {
  for (const std::pair<std::string, std::string>& pair : response) {
    std::vector<std::string> new_lines = base::SplitString(
        pair.second, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    log_lines->reserve(log_lines->size() + new_lines.size());
    log_lines->insert(log_lines->end(), new_lines.begin(), new_lines.end());
  }
}

}  // namespace

LogSourceAccessManager::LogSourceAccessManager(content::BrowserContext* context)
    : context_(context),
      tick_clock_(std::make_unique<base::DefaultTickClock>()),
      weak_factory_(this) {}

LogSourceAccessManager::~LogSourceAccessManager() {}

// static
void LogSourceAccessManager::SetRateLimitingTimeoutForTesting(
    const base::TimeDelta* timeout) {
  g_rate_limiting_timeout = timeout;
}

bool LogSourceAccessManager::FetchFromSource(
    const ReadLogSourceParams& params,
    const std::string& extension_id,
    const ReadLogSourceCallback& callback) {
  int requested_resource_id = params.reader_id ? *params.reader_id : 0;
  ApiResourceManager<LogSourceResource>* resource_manager =
      ApiResourceManager<LogSourceResource>::Get(context_);

  int resource_id = requested_resource_id > 0
                        ? requested_resource_id
                        : CreateResource(params.source, extension_id);
  if (resource_id <= 0)
    return false;

  LogSourceResource* resource =
      resource_manager->Get(extension_id, resource_id);
  if (!resource)
    return false;

  // Enforce the rules: rate-limit access to the source from the current reader
  // handle. If not enough time has elapsed since the last access, do not read
  // from the source, but instead return an empty response. From the caller's
  // perspective, there is no new data. There is no need for the caller to keep
  // track of the time since last access.
  if (!UpdateSourceAccessTime(resource_id)) {
    ReadLogSourceResult empty_result;
    callback.Run(empty_result);
    return true;
  }

  // If the API call requested a non-incremental access, clean up the
  // SingleLogSource by removing its API resource. Even if the existing source
  // were originally created as incremental, passing in incremental=false on a
  // later access indicates that the source should be closed afterwards.
  bool delete_resource_when_done = !params.incremental;

  resource->GetLogSource()->Fetch(base::Bind(
      &LogSourceAccessManager::OnFetchComplete, weak_factory_.GetWeakPtr(),
      extension_id, resource_id, delete_resource_when_done, callback));
  return true;
}

void LogSourceAccessManager::OnFetchComplete(
    const std::string& extension_id,
    int resource_id,
    bool delete_resource,
    const ReadLogSourceCallback& callback,
    SystemLogsResponse* response) {
  ReadLogSourceResult result;
  // Always return reader_id=0 if there is a cleanup.
  result.reader_id = delete_resource ? 0 : resource_id;

  GetLogLinesFromSystemLogsResponse(*response, &result.log_lines);
  if (delete_resource) {
    // This should also remove the entry from |sources_|.
    ApiResourceManager<LogSourceResource>::Get(context_)->Remove(extension_id,
                                                                 resource_id);
  }

  callback.Run(result);
}

void LogSourceAccessManager::RemoveHandle(int id) {
  open_handles_.erase(id);
}

LogSourceAccessManager::SourceAndExtension::SourceAndExtension(
    LogSource source,
    const std::string& extension_id)
    : source(source), extension_id(extension_id) {}

int LogSourceAccessManager::CreateResource(LogSource source,
                                           const std::string& extension_id) {
  // Enforce the rules: Do not create too many SingleLogSource objects to read
  // from a source, even if they are from different extensions.
  if (GetNumActiveResourcesForSource(source) >= kMaxReadersPerSource)
    return 0;

  std::unique_ptr<LogSourceResource> new_resource =
      std::make_unique<LogSourceResource>(
          extension_id,
          ExtensionsAPIClient::Get()
              ->GetFeedbackPrivateDelegate()
              ->CreateSingleLogSource(source),
          base::Bind(&LogSourceAccessManager::RemoveHandle,
                     weak_factory_.GetWeakPtr()));

  int resource_id = ApiResourceManager<LogSourceResource>::Get(context_)->Add(
      new_resource.release());
  // The resource ID isn't known until |new_resource| is added to the API
  // Resource Manager, but it needs to be passed into the resource afterward, so
  // that the resource can unregister itself from LogSourceAccessManager.
  ApiResourceManager<LogSourceResource>::Get(context_)
      ->Get(extension_id, resource_id)
      ->set_resource_id(resource_id);
  open_handles_[resource_id].reset(
      new SourceAndExtension(source, extension_id));

  return resource_id;
}

bool LogSourceAccessManager::UpdateSourceAccessTime(int id) {
  auto iter = open_handles_.find(id);
  if (iter == open_handles_.end())
    return false;

  const SourceAndExtension& key = *iter->second;
  if (rate_limiters_.find(key) == rate_limiters_.end()) {
    rate_limiters_.emplace(
        key, std::make_unique<AccessRateLimiter>(1, GetMinTimeBetweenReads(),
                                                 tick_clock_.get()));
  }
  return rate_limiters_[key]->AttemptAccess();
}

size_t LogSourceAccessManager::GetNumActiveResourcesForSource(
    LogSource source) const {
  size_t count = 0;
  for (auto iter = open_handles_.begin(); iter != open_handles_.end(); ++iter) {
    if (iter->second->source == source)
      ++count;
  }
  return count;
}

}  // namespace extensions
