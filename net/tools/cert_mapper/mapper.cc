// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/mapper.h"

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
#include "net/tools/cert_mapper/entry.h"
#include "net/tools/cert_mapper/entry_reader.h"
#include "net/tools/cert_mapper/metrics.h"
#include "net/tools/cert_mapper/visitor.h"

namespace net {

namespace {

constexpr base::TimeDelta kProgressUpdatePeriod = base::TimeDelta::FromMilliseconds(1000);

void LogProgress(const base::TimeTicks& start_time,
                 const base::TimeTicks& now,
                 size_t num_entries,
                 double progress) {
  double elapsed_sec = (now - start_time).InSecondsF();

  std::string progress_str = base::StringPrintf("%.3lf", 100.0 * progress);

  std::cerr << "\nMap progress (" << progress_str << "%)\n"
            << "    Entries read: " << num_entries << "\n"
            << "    Elapsed time: " << base::StringPrintf("%.0lf", elapsed_sec)
            << " seconds\n";
}

using Entries = std::vector<Entry>;

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

void WorkerMain(EntryReader* entry_reader,
                size_t chunk_size,
                Results* final_results,
                VisitorFactory* visitor_factory) {
  std::unique_ptr<Visitor> visitor = visitor_factory->Create();

  visitor->Start();

  Metrics local_metrics;

  std::vector<Entry> chunk;
  while (entry_reader->Read(&chunk, chunk_size)) {
    for (const auto& entry : chunk) {
      visitor->Visit(entry, &local_metrics);
    }
  }

  final_results->Merge(local_metrics);
}

void ControllerMain(EntryReader* entry_reader,
                    base::TimeTicks start_time,
                    base::TimeDelta max_elapsed_time,
                    Results* final_results) {
  double progress;
  size_t num_entries_read;
  while (entry_reader->GetProgress(&progress, &num_entries_read)) {
    base::TimeTicks now = base::TimeTicks::Now();
    LogProgress(start_time, now, num_entries_read, progress);
    if (!max_elapsed_time.is_zero() && (now - start_time) >= max_elapsed_time) {
      std::cerr << "\nABORTING because reached time limit\n";
      entry_reader->Stop();
      return;
    }

    base::PlatformThread::Sleep(kProgressUpdatePeriod);
  }

  // Log the final progress
  entry_reader->GetProgress(&progress, &num_entries_read);
  LogProgress(start_time, base::TimeTicks::Now(), num_entries_read, progress);
}

std::unique_ptr<base::Thread> StartThread(const std::string& name) {
  std::unique_ptr<base::Thread> thread = base::MakeUnique<base::Thread>(name);
  base::Thread::Options thread_options(base::MessageLoop::TYPE_IO, 0);
  CHECK(thread->StartWithOptions(thread_options));
  return thread;
}

}  // namespace

MapperOptions::MapperOptions()
    : num_samples_per_bucket(20),
      num_threads(16),
      read_chunk_size(10) {}

void ForEachEntry(EntryReader* reader,
                  VisitorFactory* visitor_factory,
                  const MapperOptions& mapper_options,
                  Metrics* metrics) {
  BucketValue::SetMaxSamples(mapper_options.num_samples_per_bucket);

  base::TimeTicks start_time = base::TimeTicks::Now();

  Results results;

  // Start the worker threads.
  std::vector<std::unique_ptr<base::Thread>> worker_threads;
  for (size_t i = 0; i < mapper_options.num_threads; ++i) {
    std::unique_ptr<base::Thread> thread =
        StartThread(base::StringPrintf("Cert Mapper %" PRIuS, i));

    thread->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&WorkerMain, base::Unretained(reader),
                   mapper_options.read_chunk_size, base::Unretained(&results),
                   base::Unretained(visitor_factory)));

    worker_threads.push_back(std::move(thread));
  }

  std::cerr << "Started " << mapper_options.num_threads << " worker threads\n";

  // Start a "controller" thread which will print the progress and abort if
  // taking too long.
  std::unique_ptr<base::Thread> controller_thread =
      StartThread("Cert Mapper Controller");

  controller_thread->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&ControllerMain, base::Unretained(reader), start_time,
                 mapper_options.max_elapsed_time, base::Unretained(&results)));

  // Wait for all the worker threads to finish.
  worker_threads.clear();

  results.CopyTo(metrics);
  metrics->Finalize();
}

}  // namespace net
