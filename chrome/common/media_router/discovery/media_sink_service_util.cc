// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/discovery/media_sink_service_util.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"

namespace media_router {

void RunSinksDiscoveredCallbackOnSequence(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const OnSinksDiscoveredCallback& callback,
    std::vector<MediaSinkInternal> sinks) {
  task_runner->PostTask(FROM_HERE, base::BindOnce(callback, std::move(sinks)));
}

void RunSinkQueryCallbackOnSequence(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const SinkQueryCallback& callback,
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks,
    const std::vector<url::Origin>& origins) {
  task_runner->PostTask(FROM_HERE,
                        base::BindOnce(callback, source_id, sinks, origins));
}

}  // namespace media_router
