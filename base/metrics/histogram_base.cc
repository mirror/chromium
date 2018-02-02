// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/histogram_base.h"

#include <limits.h>

#include <memory>
#include <set>
#include <utility>

#include "base/json/json_string_value_serializer.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/sparse_histogram.h"
#include "base/metrics/statistics_recorder.h"
#include "base/pickle.h"
#include "base/process/process_handle.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "base/values.h"

namespace base {

std::string HistogramTypeToString(HistogramType type) {
  switch (type) {
    case HISTOGRAM:
      return "HISTOGRAM";
    case LINEAR_HISTOGRAM:
      return "LINEAR_HISTOGRAM";
    case BOOLEAN_HISTOGRAM:
      return "BOOLEAN_HISTOGRAM";
    case CUSTOM_HISTOGRAM:
      return "CUSTOM_HISTOGRAM";
    case SPARSE_HISTOGRAM:
      return "SPARSE_HISTOGRAM";
  }
  NOTREACHED();
  return "UNKNOWN";
}

HistogramBase* DeserializeHistogramInfo(PickleIterator* iter) {
  int type;
  if (!iter->ReadInt(&type))
    return nullptr;

  switch (type) {
    case HISTOGRAM:
      return Histogram::DeserializeInfoImpl(iter);
    case LINEAR_HISTOGRAM:
      return LinearHistogram::DeserializeInfoImpl(iter);
    case BOOLEAN_HISTOGRAM:
      return BooleanHistogram::DeserializeInfoImpl(iter);
    case CUSTOM_HISTOGRAM:
      return CustomHistogram::DeserializeInfoImpl(iter);
    case SPARSE_HISTOGRAM:
      return SparseHistogram::DeserializeInfoImpl(iter);
    default:
      return nullptr;
  }
}

const HistogramBase::Sample HistogramBase::kSampleType_MAX = INT_MAX;

HistogramBase::HistogramBase(const char* name)
    : histogram_name_(name), flags_(kNoFlags) {}

HistogramBase::~HistogramBase() = default;

void HistogramBase::CheckName(const StringPiece& name) const {
  DCHECK_EQ(StringPiece(histogram_name()), name);
}

void HistogramBase::SetFlags(int32_t flags) {
  HistogramBase::Count old_flags = subtle::NoBarrier_Load(&flags_);
  subtle::NoBarrier_Store(&flags_, old_flags | flags);
}

void HistogramBase::ClearFlags(int32_t flags) {
  HistogramBase::Count old_flags = subtle::NoBarrier_Load(&flags_);
  subtle::NoBarrier_Store(&flags_, old_flags & ~flags);
}

void HistogramBase::AddTime(const TimeDelta& time) {
  Add(static_cast<Sample>(time.InMilliseconds()));
}

void HistogramBase::AddBoolean(bool value) {
  Add(value ? 1 : 0);
}

void HistogramBase::SerializeInfo(Pickle* pickle) const {
  pickle->WriteInt(GetHistogramType());
  SerializeInfoImpl(pickle);
}

uint32_t HistogramBase::FindCorruption(const HistogramSamples& samples) const {
  // Not supported by default.
  return NO_INCONSISTENCIES;
}

void HistogramBase::WriteHTMLGraph(std::string* output) const {
  WriteAsciiImpl(true, output);
}

void HistogramBase::WriteAscii(std::string* output) const {
  WriteAsciiImpl(false, output);
}

bool HistogramBase::ValidateHistogramContents(bool crash_if_invalid,
                                              int corrupted_count) const {
  return true;
}

void HistogramBase::WriteJSON(std::string* output,
                              JSONVerbosityLevel verbosity_level) const {
  Count count;
  int64_t sum;
  std::unique_ptr<ListValue> buckets(new ListValue());
  GetCountAndBucketData(&count, &sum, buckets.get());
  std::unique_ptr<DictionaryValue> parameters(new DictionaryValue());
  GetParameters(parameters.get());

  JSONStringValueSerializer serializer(output);
  DictionaryValue root;
  root.SetString("name", histogram_name());
  root.SetInteger("count", count);
  root.SetDouble("sum", static_cast<double>(sum));
  root.SetInteger("flags", flags());
  root.Set("params", std::move(parameters));
  if (verbosity_level != JSON_VERBOSITY_LEVEL_OMIT_BUCKETS)
    root.Set("buckets", std::move(buckets));
  root.SetInteger("pid", GetUniqueIdForProcess());
  serializer.Serialize(root);
}

void HistogramBase::FindAndRunCallback(HistogramBase::Sample sample) const {
  if ((flags() & kCallbackExists) == 0)
    return;

  StatisticsRecorder::OnSampleCallback cb =
      StatisticsRecorder::FindCallback(histogram_name());
  if (!cb.is_null())
    cb.Run(sample);
}

// static
char const* HistogramBase::GetPermanentName(const std::string& name) {
  // A set of histogram names that provides the "permanent" lifetime required
  // by histogram objects for those strings that are not already code constants
  // or held in persistent memory.
  static LazyInstance<std::set<std::string>>::Leaky permanent_names;
  static LazyInstance<Lock>::Leaky permanent_names_lock;

  AutoLock lock(permanent_names_lock.Get());
  auto result = permanent_names.Get().insert(name);
  return result.first->c_str();
}

void HistogramBase::WriteAsciiImpl(const bool is_html,
                                   std::string* const output) const {
  // Get a local copy of the data so we are consistent.
  std::unique_ptr<HistogramSamples> snapshot = SnapshotSamples();
  Count sample_count = snapshot->TotalCount();

  StringAppendF(output, is_html ? "<h2>%s</h2><pre>" : "%s\n\n",
                histogram_name());

  // If there are no samples, display the mean as 0.0 instead of NaN.
  const double mean = sample_count != 0
                          ? static_cast<double>(snapshot->sum()) / sample_count
                          : 0.0;
  StringAppendF(output, "Samples: %d, Mean: %.1f, Type: %s, Flags: 0x%x\n\n",
                sample_count, mean,
                HistogramTypeToString(GetHistogramType()).c_str(), flags());

  if (sample_count != 0) {
    *output +=
        "+------------+------------+------------+--------+------------+--------"
        "+\n"
        "|       From |         To |      Count |      % |  Cumulated |      % "
        "|\n"
        "+------------+------------+------------+--------+------------+--------"
        "+\n";

    // Determine how wide the largest bucket range is (how many digits to
    // print), so that we'll be able to right-align starts for the graphical
    // bars.
    Count count_max = 0;
    for (const std::unique_ptr<SampleCountIterator> it = snapshot->Iterator();
         !it->Done(); it->Next()) {
      Sample min;
      int64_t max;
      Count count;
      it->Get(&min, &max, &count);
      count_max = std::max(count_max, count);
    }

    const double bar_scaling = 50.0 / count_max;
    const double percent_scaling = 100.0 / sample_count;

    Count cumulated = 0;
    // Output the actual histogram graph.
    for (const std::unique_ptr<SampleCountIterator> it = snapshot->Iterator();
         !it->Done(); it->Next()) {
      Sample min;
      int64_t max;
      Count count;
      it->Get(&min, &max, &count);

      cumulated += count;
      StringAppendF(output,
                    "| %10d | %10d | %10d | %5.1f%% | %10d | %5.1f%% | ", min,
                    static_cast<Sample>(max), count, count * percent_scaling,
                    cumulated, cumulated * percent_scaling);

      output->append(count * bar_scaling, '*');
      *output += '\n';
    }

    DCHECK_EQ(sample_count, cumulated);
    *output +=
        "+------------+------------+------------+--------+------------+--------"
        "+\n";
  }

  if (is_html)
    output->append("</pre>");
}

}  // namespace base
