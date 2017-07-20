// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CT_MAPPER_MAPPER_H_
#define NET_TOOLS_CT_MAPPER_MAPPER_H_

#include <stddef.h>
#include "base/time/time.h"

namespace net {

class EntryReader;
class VisitorFactory;
class Metrics;

struct MapperOptions {
  // Initializes with default values.
  MapperOptions();

  size_t num_samples_per_bucket;
  size_t num_threads;
  size_t chunk_size;
  size_t max_pending_chunks;
  base::TimeDelta max_elapsed_time;
};

size_t ForEachEntry(EntryReader* reader,
                    VisitorFactory* visitor_factory,
                    const MapperOptions& options,
                    Metrics* metrics);

}  // namespace net

#endif  // NET_TOOLS_CT_MAPPER_MAPPER_H_
