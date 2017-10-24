// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DEMUXER_MEMORY_LIMIT_H_
#define MEDIA_BASE_DEMUXER_MEMORY_LIMIT_H_

#include <stddef.h>

#include "media/base/media_export.h"

namespace media {

// The maximum amount of data (in bytes) a demuxer can keep in memory, for a
// particular track type.
extern const size_t kDemuxerAudioMemoryLimit;
extern const size_t kDemuxerVideoMemoryLimit;

// The maximum amount of data (in bytes) a demuxer can keep in memory,
// regardless of the track type.
extern const size_t kDemuxerMemoryLimit;

}  // namespace media

#endif  // MEDIA_BASE_DEMUXER_MEMORY_LIMIT_H_
