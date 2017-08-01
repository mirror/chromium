// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/ukm_recorder.h"

#include "base/atomic_sequence_num.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"

namespace ukm {

UkmRecorder* g_ukm_recorder = nullptr;

const base::Feature kUkmFeature = {"Ukm", base::FEATURE_DISABLED_BY_DEFAULT};

UkmRecorder::UkmRecorder() = default;

UkmRecorder::~UkmRecorder() = default;

// static
void UkmRecorder::Set(UkmRecorder* recorder) {
  DCHECK(!g_ukm_recorder || !recorder);
  g_ukm_recorder = recorder;
}

// static
UkmRecorder* UkmRecorder::Get() {
  return g_ukm_recorder;
}

// static
ukm::SourceId UkmRecorder::GetNewSourceID() {
  static base::AtomicSequenceNumber seq;
  return ConvertSourceId(seq.GetNext() + 1, UkmRecorder::SourceIdType::UKM);
}

// static
ukm::SourceId UkmRecorder::ConvertSourceId(int64_t other_id,
                                           UkmRecorder::SourceIdType id_type) {
  const int64_t NUM_TYPE_BITS = 2;
  const int64_t TYPE_MASK = (INT64_C(1) << NUM_TYPE_BITS) - 1;
  const int64_t type_bits = static_cast<int64_t>(id_type);
  DCHECK_EQ(type_bits, type_bits & TYPE_MASK);
  // Stores the the type ID in the low bits of the source id, and shift the rest
  // of the ID to make room.  This could cause the original ID to overflow, but
  // that should be rare enough that it won't matter for UKM's purposes.
  return (other_id << NUM_TYPE_BITS) | type_bits;
}

std::unique_ptr<UkmEntryBuilder> UkmRecorder::GetEntryBuilder(
    ukm::SourceId source_id,
    const char* event_name) {
  return base::MakeUnique<UkmEntryBuilder>(
      base::Bind(&UkmRecorder::AddEntry, base::Unretained(this)), source_id,
      event_name);
}

}  // namespace ukm
