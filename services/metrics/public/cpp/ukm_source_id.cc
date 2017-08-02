// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/ukm_source_id.h"

#include "base/atomic_sequence_num.h"
#include "base/logging.h"

namespace ukm {

namespace {

const int64_t LOW_BITS_MASK = (INT64_C(1) << 32) - 1;
const int64_t NUM_TYPE_BITS = 2;
const int64_t TYPE_MASK = (INT64_C(1) << NUM_TYPE_BITS) - 1;

}  // namespace

SourceId AssignNewSourceId() {
  static base::AtomicSequenceNumber seq;
  return ConvertToSourceId(seq.GetNext() + 1, SourceIdType::UKM);
}

SourceId ConvertToSourceId(int64_t other_id, SourceIdType id_type) {
  const int64_t NUM_TYPE_BITS = 2;
  const int64_t TYPE_MASK = (INT64_C(1) << NUM_TYPE_BITS) - 1;
  const int64_t type_bits = static_cast<int64_t>(id_type);
  DCHECK_EQ(type_bits, type_bits & TYPE_MASK);
  // Stores the the type ID in the low bits of the source id, and shift the rest
  // of the ID to make room.  This could cause the original ID to overflow, but
  // that should be rare enough that it won't matter for UKM's purposes.
  return (other_id << NUM_TYPE_BITS) | type_bits;
}

SourceId ConvertSourceIdFromInstance(int64_t instance_id, int64_t source_id) {
  // Only convert source IDs from Create().
  if ((TYPE_MASK & source_id) != static_cast<int64_t>(SourceIdType::UKM))
    return source_id;

  // Neither ID should get large enough to cause an issue, but explicitly
  // discard down to 32 bits anyway.
  return ((instance_id & LOW_BITS_MASK) << 32) | (source_id & LOW_BITS_MASK);
}

}  // namespace ukm
