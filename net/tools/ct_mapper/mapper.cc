// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/ct_mapper/mapper.h"

#include <deque>
#include <iostream>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/format_macros.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "net/der/input.h"
#include "net/tools/ct_mapper/entry.h"
#include "net/tools/ct_mapper/entry_reader.h"
#include "net/tools/ct_mapper/metrics.h"
#include "net/tools/ct_mapper/visitor.h"

namespace net {

namespace {

// After how many milliseconds of activity to print progress update for the map.
const int kProgressTrackerUpdatePeriodMs = 1000;

class ProgressTracker {
 public:
  ProgressTracker()
      : start_time_(base::TimeTicks::Now()),
        last_logged_(base::TimeTicks::Now()),
        logging_interval_(
            base::TimeDelta::FromMilliseconds(kProgressTrackerUpdatePeriodMs)) {
  }

  base::TimeDelta GetElapsedTime(const base::TimeTicks& now) const {
    return now - start_time_;
  }

  void LogProgress(const base::TimeTicks& now,
                   size_t num_entries,
                   double progress) {
    last_logged_ = base::TimeTicks::Now();

    double elapsed_sec = GetElapsedTime(now).InSecondsF();

    std::string progress_str = base::StringPrintf("%.3lf", 100.0 * progress);

    std::cerr << "\nMap progress (" << progress_str << "%)\n"
              << "    entries processed: " << num_entries << "\n"
              << "    elapsed time: "
              << base::StringPrintf("%.3lf", elapsed_sec) << " seconds\n";
  }

  bool ShouldLogProgress(const base::TimeTicks& now) const {
    return (now - last_logged_) >= logging_interval_;
  }

 private:
  base::TimeTicks start_time_;
  base::TimeTicks last_logged_;
  base::TimeDelta logging_interval_;

  DISALLOW_COPY_AND_ASSIGN(ProgressTracker);
};

using Entries = std::vector<Entry>;

class WorkQueue {
 public:
  WorkQueue(size_t max_chunks)
      : reader_cv_(&lock_), writer_cv_(&lock_), max_chunks_(max_chunks) {}

  void AddChunk(const Entries& entries);
  bool RemoveChunk(Entries* entries);

  void Complete();

 private:
  bool CanAddChunk() { return chunks_.size() < max_chunks_; }

  bool CanRemoveChunk() { return complete_ || !chunks_.empty(); }

  base::Lock lock_;
  base::ConditionVariable reader_cv_;
  base::ConditionVariable writer_cv_;

  bool complete_ = false;

  std::deque<Entries> chunks_;
  const size_t max_chunks_;
};

void WorkQueue::AddChunk(const Entries& entries) {
  base::AutoLock l(lock_);

  DCHECK(!complete_);

  while (!CanAddChunk())
    writer_cv_.Wait();

  chunks_.push_back(entries);
  reader_cv_.Signal();
}

bool WorkQueue::RemoveChunk(Entries* entries) {
  base::AutoLock l(lock_);

  while (!CanRemoveChunk())
    reader_cv_.Wait();

  if (chunks_.empty()) {
    DCHECK(complete_);
    return false;
  }

  bool writer_is_blocked = !CanAddChunk();

  *entries = std::move(chunks_.front());
  chunks_.pop_front();

  if (writer_is_blocked && CanAddChunk())
    writer_cv_.Signal();

  return true;
}

void WorkQueue::Complete() {
  base::AutoLock l(lock_);
  complete_ = true;
  reader_cv_.Broadcast();
}

class Results {
 public:
  void Merge(const Metrics& metrics) {
    base::AutoLock l(lock_);
    metrics_.Merge(metrics);
  }

  void CopyTo(Metrics* metrics) {
    base::AutoLock l(lock_);
    *metrics = metrics_;
  }

 private:
  base::Lock lock_;
  Metrics metrics_;
};

void WorkerMain(WorkQueue* queue,
                Results* final_results,
                VisitorFactory* visitor_factory) {
  std::unique_ptr<Visitor> visitor = visitor_factory->Create();

  visitor->Start();

  Metrics local_metrics;

  Entries chunk;
  while (queue->RemoveChunk(&chunk)) {
    for (const auto& entry : chunk) {
      visitor->Visit(entry, &local_metrics);
    }
  }

  final_results->Merge(local_metrics);
}

std::vector<std::unique_ptr<base::Thread>> StartWorkerThreads(
    const MapperOptions& options,
    WorkQueue* queue,
    Results* results,
    VisitorFactory* visitor_factory) {
  std::vector<std::unique_ptr<base::Thread>> threads;
  threads.reserve(options.num_threads);

  // Initialize the worker threads.
  for (size_t i = 0; i < options.num_threads; ++i) {
    std::unique_ptr<base::Thread> thread = base::MakeUnique<base::Thread>(
        base::StringPrintf("Ct Mapper %" PRIuS, i));

    base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
    CHECK(thread->StartWithOptions(options));

    thread->task_runner()->PostTask(
        FROM_HERE, base::Bind(&WorkerMain, base::Unretained(queue),
                              base::Unretained(results),
                              base::Unretained(visitor_factory)));

    threads.push_back(std::move(thread));
  }

  return threads;
}

}  // namespace

MapperOptions::MapperOptions()
    : num_samples_per_bucket(20),
      num_threads(16),
      chunk_size(512),
      max_pending_chunks(32) {}

size_t ForEachEntry(EntryReader* reader,
                    VisitorFactory* visitor_factory,
                    const MapperOptions& options,
                    Metrics* metrics) {
  BucketValue::SetMaxSamples(options.num_samples_per_bucket);

  Results results;
  WorkQueue queue(options.max_pending_chunks);

  std::vector<std::unique_ptr<base::Thread>> worker_threads =
      StartWorkerThreads(options, &queue, &results, visitor_factory);

  ProgressTracker progress;

  size_t total_entries_read = 0;

  Entries chunk;
  while (reader->Read(&chunk, options.chunk_size)) {
    total_entries_read += chunk.size();
    queue.AddChunk(chunk);

    base::TimeTicks now = base::TimeTicks::Now();
    if (progress.ShouldLogProgress(now)) {
      progress.LogProgress(now, total_entries_read, reader->GetProgress());
    }

    if (!options.max_elapsed_time.is_zero() &&
        (progress.GetElapsedTime(now) >= options.max_elapsed_time)) {
      break;
    }
  }

  queue.Complete();

  // Wait for the workers to finish.
  worker_threads.clear();

  std::cerr << "\nDONE\n";
  progress.LogProgress(base::TimeTicks::Now(), total_entries_read, 1);
  std::cerr << "\n";

  results.CopyTo(metrics);
  metrics->Finalize();
  return total_entries_read;
}

}  // namespace net
