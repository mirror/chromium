// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_LOG_SOURCE_ACCESS_MANAGER_H_
#define EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_LOG_SOURCE_ACCESS_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "components/feedback/system_logs/system_logs_source.h"
#include "content/public/browser/browser_context.h"
#include "extensions/common/api/feedback_private.h"

namespace extensions {

// Provides bookkeepping for SingleLogSource usage. It ensures that:
// - Each extension can have only one SingleLogSource for a particular source.
// - A source may not be accessed too frequently by an extension.
class LogSourceAccessManager {
 public:
  using ReadLogSourceCallback =
      base::Callback<void(const api::feedback_private::ReadLogSourceResult&)>;

  explicit LogSourceAccessManager(content::BrowserContext* context);
  ~LogSourceAccessManager();

  // To override the default rate-limiting mechanism of this function, pass in
  // a TimeDelta representing the desired minimum time between consecutive reads
  // of a source from an extension. Does not take ownership of |timeout|. When
  // done testing, call this function again with |timeout|=nullptr to reset to
  // the default behavior.
  static void SetRateLimitingTimeoutForTesting(const base::TimeDelta* timeout);

  // Override the default base::Time clock with a custom clock for testing.
  // Pass in |clock|=nullptr to revert to default behavior.
  void SetTickClockForTesting(std::unique_ptr<base::TickClock> clock) {
    tick_clock_ = std::move(clock);
  }

  // Initiates a fetch from a log source, as specified in |params|. See
  // feedback_private.idl for more info about the actual parameters.
  bool FetchFromSource(const api::feedback_private::ReadLogSourceParams& params,
                       const std::string& extension_id,
                       const ReadLogSourceCallback& callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(LogSourceAccessManagerTest,
                           MaxNumberOfOpenLogSourcesSameExtension);
  FRIEND_TEST_ALL_PREFIXES(LogSourceAccessManagerTest,
                           MaxNumberOfOpenLogSourcesDifferentExtensions);

  // Contains info about an open log source handle.
  struct LogSourceHandle {
    explicit LogSourceHandle(api::feedback_private::LogSource source,
                             const std::string& extension_id);

    // The log source that this handle is accessing.
    api::feedback_private::LogSource source;
    // ID of the extension that opened this handle.
    std::string extension_id;
    // Timestamp of when this handle was last used to access the log source.
    base::TimeTicks last_access_time;
  };

  // Creates a new LogSourceResource for the source and extension indicated by
  // |source| and |extension_id|. Stores the new resource in the API Resource
  // Manager, and generates a new LogSourceHandle containing |source| and
  // |extension_id| in |open_handles_| as a new entry.
  //
  // Returns the nonzero ID of the newly created LogSourceResource, or 0 if a
  // new resource could be created.
  int CreateResource(api::feedback_private::LogSource source,
                     const std::string& extension_id);

  // Callback that is passed to the log source from FetchFromSource.
  // Arguments:
  // - extension_id: ID of extension that opened the log source.
  // - resource_id: Resource ID provided by API Resource Manager for the reader.
  // - delete_source: Set this if the source opened by |handle| should be
  //   removed from both the API Resource Manager and from |open_handles_|.
  // - response_callback: Callback for sending the response as a
  //   ReadLogSourceResult struct.
  void OnFetchComplete(const std::string& extension_id,
                       int resource_id,
                       bool delete_source,
                       const ReadLogSourceCallback& callback,
                       system_logs::SystemLogsResponse* response);

  // Removes an existing log source handle indicated by |id| from
  // |open_handles_|.
  void RemoveHandle(int id);

  // Attempts to update the |last_access_time| field for the LogSourceHandle
  // |open_handles[id]|, to record that the source is being accessed by the
  // handle right now. If less than |min_time_between_reads_| has elapsed since
  // the last successful read, does not update |last_access_times|, and instead
  // returns false. Otherwise returns true.
  bool UpdateSourceAccessTime(int id);

  // Returns the number of entries in |open_handles_| with source=|source|.
  size_t GetNumActiveResourcesForSource(
      api::feedback_private::LogSource source) const;

  // Contains all open LogSourceHandles.
  std::map<int, std::unique_ptr<LogSourceHandle>> open_handles_;

  // For fetching browser resources like ApiResourceManager.
  content::BrowserContext* context_;

  // Provides a timer clock implementation for keeping track of access times.
  // Can override the default clock for testing.
  std::unique_ptr<base::TickClock> tick_clock_;

  base::WeakPtrFactory<LogSourceAccessManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LogSourceAccessManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_LOG_SOURCE_ACCESS_MANAGER_H_
