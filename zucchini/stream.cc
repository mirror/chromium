// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/stream.h"

namespace zucchini {

/******** SinkStreamSet ********/

SinkStreamSet::SinkStreamSet() = default;

SinkStreamSet::SinkStreamSet(size_t count) : data_(count) {}

SinkStreamSet::~SinkStreamSet() = default;

/******** SourceStreamSet ********/

SourceStreamSet::SourceStreamSet() = default;

SourceStreamSet::SourceStreamSet(const SourceStreamSet&) = default;

SourceStreamSet::~SourceStreamSet() = default;

bool SourceStreamSet::Init(Stream* src) {
  uint32_t ss_count = 0;
  if (!(*src)(&ss_count) || ss_count > kMaxSourceStreamCount)
    return false;
  data_.resize(ss_count);
  // Read all substream sizes.
  for (Substream& ss : data_) {
    if (!(*src)(&ss.size))
      return false;
  }
  // Assign all substream iterators.
  for (Substream& ss : data_) {
    ss.it = src->current();
    if (!src->skip(ss.size))
      return false;
  }
  return true;
}

}  // namespace zucchini
