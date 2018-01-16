// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_SERVICE_UTIL_H_
#define CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_SERVICE_UTIL_H_

#include <vector>

#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "chrome/common/media_router/media_source.h"
#include "url/origin.h"

namespace base {
class SequencedTaskRunner;
}

namespace media_router {

using OnSinksDiscoveredCallback =
    base::RepeatingCallback<void(std::vector<MediaSinkInternal>)>;

// Runs |sinks_discovered_cb| with |sinks| on |task_runner|.
void RunSinksDiscoveredCallbackOnSequence(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const OnSinksDiscoveredCallback& callback,
    std::vector<MediaSinkInternal> sinks);

// |source_id|: The source of the query.
// |sinks|: The list of sinks compatible with |source_id|.
// |origins|: If non-empty, list of origins for which the query is valid.
using SinkQueryCallback =
    base::RepeatingCallback<void(const MediaSource::Id& source_id,
                                 const std::vector<MediaSinkInternal>& sinks,
                                 const std::vector<url::Origin>& origins)>;

void RunSinkQueryCallbackOnSequence(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const SinkQueryCallback& callback,
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks,
    const std::vector<url::Origin>& origins);

}  // namespace media_router

#endif  // CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_SERVICE_UTIL_H_
