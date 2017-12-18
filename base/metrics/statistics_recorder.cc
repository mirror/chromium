// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/statistics_recorder.h"

#include <memory>

#include "base/at_exit.h"
#include "base/debug/leak_annotations.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_snapshot_manager.h"
#include "base/metrics/metrics_hashes.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/metrics/record_histogram_checker.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"

namespace base {
namespace {

LazyInstance<Lock>::Leaky g_top_lock;

// Current statistics recorder.
StatisticsRecorder* g_top = nullptr;

bool HistogramNameLesser(const base::HistogramBase* a,
                         const base::HistogramBase* b) {
  return strcmp(a->histogram_name(), b->histogram_name()) < 0;
}

}  // namespace

// static
bool StatisticsRecorder::vlog_initialized_ = false;

size_t StatisticsRecorder::BucketRangesHash::operator()(
    const BucketRanges* const a) const {
  return a->checksum();
}

bool StatisticsRecorder::BucketRangesEqual::operator()(
    const BucketRanges* const a,
    const BucketRanges* const b) const {
  return a->Equals(b);
}

StatisticsRecorder::~StatisticsRecorder() {
  const AutoLock lock(g_top_lock.Get());
  DCHECK_EQ(this, g_top);
  g_top = previous_;
}

// static
void StatisticsRecorder::Initialize() {
  const AutoLock lock(g_top_lock.Get());
  if (g_top)
    return;

  const StatisticsRecorder* const p = new StatisticsRecorder;
  ANNOTATE_LEAKING_OBJECT_PTR(p);
  DCHECK_EQ(p, g_top);
}

// static
bool StatisticsRecorder::IsActive() {
  const AutoLock lock(g_top_lock.Get());
  return g_top != nullptr;
}

// static
void StatisticsRecorder::RegisterHistogramProvider(
    const WeakPtr<HistogramProvider>& provider) {
  const AutoLock lock(g_top_lock.Get());
  g_top->providers_.push_back(provider);
}

// static
HistogramBase* StatisticsRecorder::RegisterOrDeleteDuplicate(
    HistogramBase* histogram) {
  // Declared before auto_lock to ensure correct destruction order.
  std::unique_ptr<HistogramBase> histogram_deleter;
  const AutoLock lock(g_top_lock.Get());

  if (!g_top) {
    // As per crbug.com/79322 the histograms are intentionally leaked, so we
    // need to annotate them. Because ANNOTATE_LEAKING_OBJECT_PTR may be used
    // only once for an object, the duplicates should not be annotated.
    // Callers are responsible for not calling RegisterOrDeleteDuplicate(ptr)
    // twice |if (!g_top)|.
    ANNOTATE_LEAKING_OBJECT_PTR(histogram);  // see crbug.com/79322
    return histogram;
  }

  const char* const name = histogram->histogram_name();
  HistogramBase*& registered = g_top->histograms_[name];

  if (!registered) {
    // |name| is guaranteed to never change or be deallocated so long
    // as the histogram is alive (which is forever).
    registered = histogram;
    ANNOTATE_LEAKING_OBJECT_PTR(histogram);  // see crbug.com/79322
    // If there are callbacks for this histogram, we set the kCallbackExists
    // flag.
    const auto callback_iterator = g_top->callbacks_.find(name);
    if (callback_iterator != g_top->callbacks_.end()) {
      if (!callback_iterator->second.is_null())
        histogram->SetFlags(HistogramBase::kCallbackExists);
      else
        histogram->ClearFlags(HistogramBase::kCallbackExists);
    }
    return histogram;
  }

  if (histogram == registered) {
    // The histogram was registered before.
    return histogram;
  }

  // We already have one histogram with this name.
  histogram_deleter.reset(histogram);
  return registered;
}

// static
const BucketRanges* StatisticsRecorder::RegisterOrDeleteDuplicateRanges(
    const BucketRanges* ranges) {
  DCHECK(ranges->HasValidChecksum());

  // Declared before auto_lock to ensure correct destruction order.
  std::unique_ptr<const BucketRanges> ranges_deleter;
  const AutoLock lock(g_top_lock.Get());

  if (!g_top) {
    ANNOTATE_LEAKING_OBJECT_PTR(ranges);
    return ranges;
  }

  const BucketRanges* const registered = *g_top->ranges_.insert(ranges).first;
  if (registered == ranges) {
    ANNOTATE_LEAKING_OBJECT_PTR(ranges);
  } else {
    ranges_deleter.reset(ranges);
  }

  return registered;
}

// static
void StatisticsRecorder::WriteHTMLGraph(const std::string& query,
                                        std::string* output) {
  Histograms snapshot;
  GetSnapshot(query, &snapshot);
  std::sort(snapshot.begin(), snapshot.end(), &HistogramNameLesser);
  for (const HistogramBase* histogram : snapshot) {
    histogram->WriteHTMLGraph(output);
    *output += "<br><hr><br>";
  }
}

// static
void StatisticsRecorder::WriteGraph(const std::string& query,
                                    std::string* output) {
  if (query.length())
    StringAppendF(output, "Collections of histograms for %s\n", query.c_str());
  else
    output->append("Collections of all histograms\n");

  Histograms snapshot;
  GetSnapshot(query, &snapshot);
  std::sort(snapshot.begin(), snapshot.end(), &HistogramNameLesser);
  for (const HistogramBase* histogram : snapshot) {
    histogram->WriteAscii(output);
    output->append("\n");
  }
}

// static
std::string StatisticsRecorder::ToJSON(JSONVerbosityLevel verbosity_level) {
  Histograms snapshot;
  GetSnapshot(std::string(), &snapshot);

  std::string output = "{\"histograms\":[";
  const char* sep = "";
  for (const HistogramBase* const histogram : snapshot) {
    output += sep;
    sep = ",";
    std::string json;
    histogram->WriteJSON(&json, verbosity_level);
    output += json;
  }
  output += "]}";
  return output;
}

// static
void StatisticsRecorder::GetHistograms(Histograms* output) {
  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return;

  for (const auto& entry : g_top->histograms_) {
    output->push_back(entry.second);
  }
}

// static
void StatisticsRecorder::GetBucketRanges(
    std::vector<const BucketRanges*>* output) {
  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return;

  for (const BucketRanges* const p : g_top->ranges_) {
    output->push_back(p);
  }
}

// static
HistogramBase* StatisticsRecorder::FindHistogram(base::StringPiece name) {
  // This must be called *before* the lock is acquired below because it will
  // call back into this object to register histograms. Those called methods
  // will acquire the lock at that time.
  ImportGlobalPersistentHistograms();

  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return nullptr;

  const HistogramMap::const_iterator it = g_top->histograms_.find(name);
  return it != g_top->histograms_.end() ? it->second : nullptr;
}

// static
StatisticsRecorder::HistogramProviders
StatisticsRecorder::GetHistogramProviders() {
  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return {};

  return g_top->providers_;
}

// static
void StatisticsRecorder::ImportProvidedHistograms() {
  // Merge histogram data from each provider in turn.
  for (const WeakPtr<HistogramProvider>& provider : GetHistogramProviders()) {
    // Weak-pointer may be invalid if the provider was destructed, though they
    // generally never are.
    if (provider)
      provider->MergeHistogramDeltas();
  }
}

// static
void StatisticsRecorder::PrepareDeltas(
    bool include_persistent,
    HistogramBase::Flags flags_to_set,
    HistogramBase::Flags required_flags,
    HistogramSnapshotManager* snapshot_manager) {
  if (include_persistent)
    ImportGlobalPersistentHistograms();

  Histograms known = GetKnownHistograms(include_persistent);
  std::sort(known.begin(), known.end(), &HistogramNameLesser);
  snapshot_manager->PrepareDeltas(known, flags_to_set, required_flags);
}

// static
void StatisticsRecorder::InitLogOnShutdown() {
  const AutoLock lock(g_top_lock.Get());
  InitLogOnShutdownWithoutLock();
}

// static
void StatisticsRecorder::GetSnapshot(const std::string& query,
                                     Histograms* snapshot) {
  // This must be called *before* the lock is acquired below because it will
  // call back into this object to register histograms. Those called methods
  // will acquire the lock at that time.
  ImportGlobalPersistentHistograms();

  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return;

  // Need a c-string query for comparisons against c-string histogram name.
  const char* query_string = query.c_str();

  for (const auto& entry : g_top->histograms_) {
    if (strstr(entry.second->histogram_name(), query_string) != nullptr)
      snapshot->push_back(entry.second);
  }
}

// static
bool StatisticsRecorder::SetCallback(
    const std::string& name,
    const StatisticsRecorder::OnSampleCallback& cb) {
  DCHECK(!cb.is_null());
  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return false;

  if (!g_top->callbacks_.insert({name, cb}).second)
    return false;

  const HistogramMap::const_iterator it = g_top->histograms_.find(name);
  if (it != g_top->histograms_.end())
    it->second->SetFlags(HistogramBase::kCallbackExists);

  return true;
}

// static
void StatisticsRecorder::ClearCallback(const std::string& name) {
  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return;

  g_top->callbacks_.erase(name);

  // We also clear the flag from the histogram (if it exists).
  const HistogramMap::const_iterator it = g_top->histograms_.find(name);
  if (it != g_top->histograms_.end())
    it->second->ClearFlags(HistogramBase::kCallbackExists);
}

// static
StatisticsRecorder::OnSampleCallback StatisticsRecorder::FindCallback(
    const std::string& name) {
  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return OnSampleCallback();

  const auto it = g_top->callbacks_.find(name);
  return it != g_top->callbacks_.end() ? it->second : OnSampleCallback();
}

// static
size_t StatisticsRecorder::GetHistogramCount() {
  const AutoLock lock(g_top_lock.Get());
  return g_top ? g_top->histograms_.size() : 0;
}

// static
void StatisticsRecorder::ForgetHistogramForTesting(base::StringPiece name) {
  const AutoLock lock(g_top_lock.Get());
  if (!g_top)
    return;

  const HistogramMap::iterator found = g_top->histograms_.find(name);
  if (found == g_top->histograms_.end())
    return;

  HistogramBase* const base = found->second;
  if (base->GetHistogramType() != SPARSE_HISTOGRAM) {
    // When forgetting a histogram, it's likely that other information is
    // also becoming invalid. Clear the persistent reference that may no
    // longer be valid. There's no danger in this as, at worst, duplicates
    // will be created in persistent memory.
    static_cast<Histogram*>(base)->bucket_ranges()->set_persistent_reference(0);
  }

  g_top->histograms_.erase(found);
}

// static
std::unique_ptr<StatisticsRecorder>
StatisticsRecorder::CreateTemporaryForTesting() {
  const AutoLock lock(g_top_lock.Get());
  return WrapUnique(new StatisticsRecorder());
}

// static
void StatisticsRecorder::SetRecordChecker(
    std::unique_ptr<RecordHistogramChecker> record_checker) {
  const AutoLock lock(g_top_lock.Get());
  g_top->record_checker_ = std::move(record_checker);
}

// static
bool StatisticsRecorder::ShouldRecordHistogram(uint64_t histogram_hash) {
  const AutoLock lock(g_top_lock.Get());
  return !g_top || !g_top->record_checker_ ||
         g_top->record_checker_->ShouldRecord(histogram_hash);
}

// static
StatisticsRecorder::Histograms StatisticsRecorder::GetKnownHistograms(
    bool include_persistent) {
  Histograms known;
  const AutoLock lock(g_top_lock.Get());
  if (!g_top || g_top->histograms_.empty())
    return known;

  known.reserve(g_top->histograms_.size());
  for (const auto& h : g_top->histograms_) {
    if (include_persistent ||
        (h.second->flags() & HistogramBase::kIsPersistent) == 0)
      known.push_back(h.second);
  }

  return known;
}

// static
void StatisticsRecorder::ImportGlobalPersistentHistograms() {
  // Import histograms from known persistent storage. Histograms could have been
  // added by other processes and they must be fetched and recognized locally.
  // If the persistent memory segment is not shared between processes, this call
  // does nothing.
  if (GlobalHistogramAllocator* allocator = GlobalHistogramAllocator::Get())
    allocator->ImportHistogramsToStatisticsRecorder();
}

// This singleton instance should be started during the single threaded portion
// of main(), and hence it is not thread safe. It initializes globals to provide
// support for all future calls.
StatisticsRecorder::StatisticsRecorder() {
  g_top_lock.Get().AssertAcquired();
  previous_ = g_top;
  g_top = this;
  InitLogOnShutdownWithoutLock();
}

// static
void StatisticsRecorder::InitLogOnShutdownWithoutLock() {
  g_top_lock.Get().AssertAcquired();
  if (!vlog_initialized_ && VLOG_IS_ON(1)) {
    vlog_initialized_ = true;
    AtExitManager::RegisterCallback(&DumpHistogramsToVlog, nullptr);
  }
}

// static
void StatisticsRecorder::DumpHistogramsToVlog(void*) {
  std::string output;
  WriteGraph("", &output);
  VLOG(1) << output;
}

}  // namespace base
