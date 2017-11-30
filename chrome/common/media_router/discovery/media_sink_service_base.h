// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_SERVICE_BASE_H_
#define CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_SERVICE_BASE_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "chrome/common/media_router/discovery/media_sink_service_util.h"

namespace media_router {

// Base class for discovering MediaSinks and notifying the caller with updated
// list. The class throttles invocations of the callback with a fixed timeout
// when multiple updates happen in quick succession.
// This class is not thread safe. This class may be created on any thread.
// All methods must be invoked on the same thread.
class MediaSinkServiceBase {
 public:
  // |callback|: The callback to invoke when the list of MediaSinks has been
  // updated.
  explicit MediaSinkServiceBase(const OnSinksDiscoveredCallback& callback);
  virtual ~MediaSinkServiceBase();

  // Forces |callback| to be invoked with the latest sink list.
  // Marked virtual for tests.
  virtual void ForceSinkDiscoveryCallback();

 protected:
  void SetTimerForTest(std::unique_ptr<base::Timer> timer);

  // Called when |discovery_timer_| expires.
  virtual void OnDiscoveryComplete();

  // Overriden by subclass to report device counts.
  virtual void RecordDeviceCounts() {}

  // Helper function to start |discovery_timer_|. Create a new timer if none
  // exists.
  void StartTimer();

  // Helper function to restart |discovery_timer| if it is stopped. Can only be
  // called after |StartTimer()| is called.
  void RestartTimer();

  // Sorted sinks from current round of discovery.
  base::flat_set<MediaSinkInternal> current_sinks_;

 private:
  friend class MediaSinkServiceBaseTest;
  FRIEND_TEST_ALL_PREFIXES(MediaSinkServiceBaseTest,
                           TestOnDiscoveryComplete_SameSink);

  // Time out value for |discovery_timer_|.
  int discovery_timer_secs_;

  OnSinksDiscoveredCallback on_sinks_discovered_cb_;

  // Sorted sinks sent to Media Router Provider in last |OnDiscoveryComplete()|
  base::flat_set<MediaSinkInternal> mrp_sinks_;

  // Timer for completing the current round of discovery.
  std::unique_ptr<base::Timer> discovery_timer_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(MediaSinkServiceBase);
};

}  // namespace media_router

#endif  // CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_SERVICE_BASE_H_
