// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_APP_AVAILABILITY_TRACKER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_APP_AVAILABILITY_TRACKER_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "chrome/browser/media/router/providers/cast/parsed_media_source.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source.h"

namespace media_router {

// Possible app availability values.
// TODO(imcheng): Implement RESCAN statuses.
enum class AppAvailability { kUnavailable, kAvailable };

// Tracks sink queries and their extracted Cast app IDs and their availabilities
// on discovered sinks.
class AppAvailabilityTracker {
 public:
  AppAvailabilityTracker();
  ~AppAvailabilityTracker();

  // Returns |true| if |source| is registered.
  bool HasSource(const ParsedMediaSource& source) const;

  // Registers |source| with the tracker. Returns a list of new app IDs that
  // were previously not known to the tracker.
  base::flat_set<std::string> RegisterSource(const ParsedMediaSource& source);

  // Unregisters the source given by |source_id| with the tracker.
  void UnregisterSource(const MediaSource::Id& source_id);

  // Updates the availability of |app_id| on |sink_id| with |availability|.
  // Returns a list of ParsedMediaSources for which the set of available sinks
  // might have been updated by this call.
  std::vector<ParsedMediaSource> UpdateAppAvailability(
      const MediaSink::Id& sink_id,
      const std::string& app_id,
      AppAvailability availability);

  // Removes all results associated with |sink_id|, i.e. when the sink becomes
  // invalid.
  // Returns a list of ParsedMediaSources for which the set of available sinks
  // might have been updated by this call.
  std::vector<ParsedMediaSource> RemoveResultsForSink(
      const MediaSink::Id& sink_id);

  // Returns whether availability status is known for |app_id| on |sink_id|.
  bool IsAvailabilityKnown(const MediaSink::Id& sink_id,
                           const std::string& app_id) const;

  // Returns a list of registered app IDs.
  base::flat_set<std::string> GetRegisteredApps() const;

  // Returns a list of sink IDs compatible with |source|, using the current
  // availability info.
  base::flat_set<MediaSink::Id> GetAvailableSinks(
      const ParsedMediaSource& source) const;

 private:
  using AppAvailabilityMap = base::flat_map<std::string, AppAvailability>;
  base::flat_map<MediaSource::Id, ParsedMediaSource> registered_sources_;
  base::flat_map<std::string, int> observer_count_by_app_id_;
  base::flat_map<MediaSink::Id, AppAvailabilityMap> app_availabilities_;
  base::flat_map<std::string, base::flat_set<MediaSink::Id>>
      available_sinks_by_app_id_;

  DISALLOW_COPY_AND_ASSIGN(AppAvailabilityTracker);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_APP_AVAILABILITY_TRACKER_H_
