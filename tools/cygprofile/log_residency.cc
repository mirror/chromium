// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include "base/files/file.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "tools/cygprofile/anchor_functions.h"

namespace cygprofile {
namespace {

// Timestamp in ns since Unix Epoch, and residency, as returned by mincore().
struct TimestampAndResidency {
  uint64_t timestamp_nanos;
  std::vector<unsigned char> residency;

  TimestampAndResidency(uint64_t timestamp_nanos,
                        std::vector<unsigned char> residency)
      : timestamp_nanos(timestamp_nanos), residency(residency) {}
};

// Records residency every 500ms for ~30s, then writes the timeline to disk.
class ResidencyRecorder {
 public:
  ResidencyRecorder() {
    CheckOrderingSanity();
    std::thread(&ResidencyRecorder::PeriodicallyCollectResidency, this)
        .detach();
  }

 private:
  void PeriodicallyCollectResidency() {
    residency_data_ = std::make_unique<std::vector<TimestampAndResidency>>();

    // Though executable code start has to be aligned on a page boundary, this
    // is not necessarily the case for .text, as .plt typically precedes it.
    // Align start and size to page boundaries.
    size_t text_start_page_aligned = kStartOfText & ~(kPageSize - 1);
    size_t size = kEndOfText - text_start_page_aligned;
    size_t text_size_in_pages = size / kPageSize + (size % kPageSize ? 1 : 0);

    for (int i = 0; i < 60; ++i) {
      CollectResidency(reinterpret_cast<void*>(text_start_page_aligned),
                       text_size_in_pages * kPageSize);
      usleep(5e5);
    }
    DumpResidency();
  }

  void CollectResidency(void* start, size_t size) {
    // Not using base::TimeTicks() to not call too many base:: symbol that would
    // pollute the reached symbols dumps.
    struct timespec ts;
    if (HANDLE_EINTR(clock_gettime(CLOCK_MONOTONIC, &ts))) {
      PLOG(ERROR) << "Cannot get the time.";
      return;
    }
    uint64_t now = ts.tv_sec * 1000000000 + ts.tv_nsec;
    auto data = std::vector<unsigned char>(size / kPageSize);
    int err = HANDLE_EINTR(mincore(start, size, &data[0]));
    if (err) {
      PLOG(ERROR) << "mincore() failed";
      return;
    }

    residency_data_->emplace_back(now, std::move(data));
  }

  void DumpResidency() {
    auto data = std::move(residency_data_);
    auto path = base::FilePath(base::StringPrintf(
        "/data/local/tmp/chrome/residency-%d.txt", getpid()));
    auto file = base::File(
        path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!file.IsValid()) {
      // This is expected in renderer processes, as they don't have access to
      // the file system.
      LOG(WARNING) << "Cannot open the file";
      return;
    }

    for (const auto& data_point : *data) {
      auto timestamp =
          base::StringPrintf("%" PRIu64 " ", data_point.timestamp_nanos);
      file.WriteAtCurrentPos(timestamp.c_str(), timestamp.size());

      std::vector<char> dump;
      dump.reserve(data_point.residency.size() + 1);
      for (auto c : data_point.residency)
        dump.push_back(c ? '1' : '0');
      dump[dump.size() - 1] = '\n';
      file.WriteAtCurrentPos(&dump[0], dump.size());
    }
  }

  std::unique_ptr<std::vector<TimestampAndResidency>> residency_data_;
  static constexpr size_t kPageSize = 1 << 12;
};

// Static constructor, to ensure that recording starts early.
static ResidencyRecorder recorder;

}  // namespace
}  // namespace cygprofile
