// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_MAPPER_H_
#define NET_TOOLS_CERT_MAPPER_MAPPER_H_

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
  size_t read_chunk_size;
  base::TimeDelta max_elapsed_time;
};

void ForEachEntry(EntryReader* reader,
                  VisitorFactory* visitor_factory,
                  const MapperOptions& options,
                  Metrics* metrics);

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_MAPPER_H_
