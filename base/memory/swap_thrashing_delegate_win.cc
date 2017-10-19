// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_delegate_win.h"

#include <windows.h>
#include <winternl.h>

#include "base/win/win_util.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"

namespace base {

// The polling interval, in milliseconds.
const int kPollingIntervalMs = 2000;

// The period of time during which swap-thrashing has to be observed in order
// to switch from the SWAP_THRASHING_LEVEL_NONE to the
// SWAP_THRASHING_LEVEL_SUSPECTED level, in milliseconds.
const int kNoneToSuspectedIntervalMs = 8000;

// The period of time during which swap-thrashing should not be observed in
// order to switch from the SWAP_THRASHING_LEVEL_SUSPECTED to the
// SWAP_THRASHING_LEVEL_NONE level, in milliseconds.
const int kSuspectedIntervalCoolDownMs = 12000;

// The period of time during which swap-thrashing has to be observed while
// being in the SWAP_THRASHING_LEVEL_SUSPECTED state in order to switch to the
// SWAP_THRASHING_LEVEL_CONFIRMED level, in milliseconds.
const int kSuspectedToConfirmedIntervalMs = 14000;

// The period of time during which swap-thrashing should not be observed in
// order to switch from the SWAP_THRASHING_LEVEL_CONFIRMED to the
// SWAP_THRASHING_LEVEL_SUSPECTED level, in milliseconds.
const int kConfirmedCoolDownMs = 20000;

// The average hard page-fault rate (/sec) that we expect to see when switching
// to a higher swap-thrashing level.
const size_t kThrashSwappingPageFaultThreshold = 5;

// The average hard page-fault rate (/sec) that we use to go down to a lower
// swap-thrashing level.
const size_t kPageFaultCooldownThreshold = 3;

namespace {

using SwapThrashingLevel = SwapThrashingMonitorDelegateWin::SwapThrashingLevel;

const size_t kNoneToSuspectedSampleCount =
    kNoneToSuspectedIntervalMs / kPollingIntervalMs;
const size_t kSuspectedIntervalCoolDownSampleCount =
    kSuspectedIntervalCoolDownMs / kPollingIntervalMs;
const size_t kSuspectedToConfirmedSampleCount =
    kSuspectedToConfirmedIntervalMs / kPollingIntervalMs;
const size_t kConfirmedCoolDownSampleCount =
    kConfirmedCoolDownMs / kPollingIntervalMs;

// The signature of the NtQuerySystemInformation function.
typedef NTSTATUS(WINAPI* NtQuerySystemInformationPtr)(SYSTEM_INFORMATION_CLASS,
                                                      PVOID,
                                                      ULONG,
                                                      PULONG);

// The struct used to return system process information via the NT internal
// QuerySystemInformation call. This is partially documented at
// http://goo.gl/Ja9MrH and fully documented at http://goo.gl/QJ70rn
// This structure is laid out in the same format on both 32-bit and 64-bit
// systems, but has a different size due to the various pointer-sized fields.
struct SYSTEM_PROCESS_INFORMATION_EX {
  ULONG NextEntryOffset;
  ULONG NumberOfThreads;
  LARGE_INTEGER WorkingSetPrivateSize;
  ULONG HardFaultCount;
  ULONG NumberOfThreadsHighWatermark;
  ULONGLONG CycleTime;
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER KernelTime;
  UNICODE_STRING ImageName;
  LONG KPriority;
  // This is labeled a handle so that it expands to the correct size for 32-bit
  // and 64-bit operating systems. However, under the hood it's a 32-bit DWORD
  // containing the process ID.
  HANDLE UniqueProcessId;
  PVOID Reserved3;
  ULONG HandleCount;
  BYTE Reserved4[4];
  PVOID Reserved5[11];
  SIZE_T PeakPagefileUsage;
  SIZE_T PrivatePageCount;
  LARGE_INTEGER Reserved6[6];
  // Array of SYSTEM_THREAD_INFORMATION structs follows.
};

// Gets the hard fault count of the current process through |hard_fault_count|.
// Returns true on success.
bool GetHardFaultCountForChromeProcesses(uint64_t* hard_fault_count) {
  DCHECK(hard_fault_count);

  *hard_fault_count = 0;

  // Get the function pointer.
  static const NtQuerySystemInformationPtr query_sys_info =
      reinterpret_cast<NtQuerySystemInformationPtr>(::GetProcAddress(
          GetModuleHandle(L"ntdll.dll"), "NtQuerySystemInformation"));
  if (query_sys_info == nullptr)
    return false;

  // The output of this system call depends on the number of threads and
  // processes on the entire system, and this can change between calls. Retry
  // a small handful of times growing the buffer along the way.
  // NOTE: The actual required size depends entirely on the number of processes
  //       and threads running on the system. The initial guess suffices for
  //       ~100s of processes and ~1000s of threads.
  std::vector<uint8_t> buffer(32 * 1024);
  for (size_t tries = 0; tries < 3; ++tries) {
    ULONG return_length = 0;
    const NTSTATUS status =
        query_sys_info(SystemProcessInformation, buffer.data(),
                       static_cast<ULONG>(buffer.size()), &return_length);
    // Insufficient space in the buffer.
    if (return_length > buffer.size()) {
      buffer.resize(return_length);
      continue;
    }
    if (NT_SUCCESS(status) && return_length <= buffer.size())
      break;
    return false;
  }

  // Look for the struct housing information for the Chrome processes.
  size_t index = 0;
  while (index < buffer.size()) {
    DCHECK_LE(index + sizeof(SYSTEM_PROCESS_INFORMATION_EX), buffer.size());
    SYSTEM_PROCESS_INFORMATION_EX* proc_info =
        reinterpret_cast<SYSTEM_PROCESS_INFORMATION_EX*>(buffer.data() + index);
    if (base::FilePath::CompareEqualIgnoreCase(proc_info->ImageName.Buffer,
                                               L"chrome.exe")) {
      *hard_fault_count += proc_info->HardFaultCount;
      return true;
    }
    // The list ends when NextEntryOffset is zero. This also prevents busy
    // looping if the data is in fact invalid.
    if (proc_info->NextEntryOffset <= 0)
      return false;
    index += proc_info->NextEntryOffset;
  }

  return true;
}

}  // namespace

SwapThrashingMonitorDelegateWin::SwapThrashingMonitorDelegateWin()
    : swap_thrashing_observation_window_(kNoneToSuspectedSampleCount),
      swap_thrashing_cooldown_window_(0U),
      previous_observation_(0U),
      previous_observation_is_valid_(false) {}

SwapThrashingMonitorDelegateWin::~SwapThrashingMonitorDelegateWin() {}

int SwapThrashingMonitorDelegateWin::GetPollingIntervalMs() {
  return kPollingIntervalMs;
}

SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
    PageFaultDetectionWindow(size_t window_size)
    : window_size_(window_size), average_(0U) {
  ResetWindow(window_size);
}

SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
    ~PageFaultDetectionWindow() {}

void SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::ResetWindow(
    size_t new_window_size) {
  window_size_ = new_window_size;
  page_fault_counts_.clear();
  if (window_size_ != 0U)
    page_fault_counts_.reserve(new_window_size);
}

void SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::AddSample(
    uint64_t sample) {
  if (window_size_ == 0)
    return;
  size_t samples_sum =
      static_cast<size_t>(average_ * page_fault_counts_.size());
  if (page_fault_counts_.size() == window_size_) {
    samples_sum -= page_fault_counts_.front();
    page_fault_counts_.pop_front();
  }
  page_fault_counts_.push_back(sample);
  samples_sum += sample;
  average_ = static_cast<double>(samples_sum) / page_fault_counts_.size();
}

bool SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::IsReady()
    const {
  if (window_size_ == 0 || page_fault_counts_.size() < window_size_)
    return false;
  return true;
}

double SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::GetAverage()
    const {
  static const size_t kOneSecToMs = 1000;
  return (average_ * kOneSecToMs) / kPollingIntervalMs;
}

SwapThrashingLevel
SwapThrashingMonitorDelegateWin::CalculateCurrentSwapThrashingLevel(
    SwapThrashingMonitorDelegateWin::SwapThrashingLevel previous_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  uint64_t hard_fault_count = 0;
  if (!GetHardFaultCountForChromeProcesses(&hard_fault_count)) {
    LOG(ERROR) << "Unable to retrieve the hard fault counts for Chrome "
               << "processes.";
    return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
  }

  if (!previous_observation_is_valid_) {
    previous_observation_is_valid_ = true;
    previous_observation_ = hard_fault_count;
    return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
  }

  uint64_t hard_fault_delta = 0;
  if (hard_fault_count >= previous_observation_) {
    hard_fault_delta = hard_fault_count - previous_observation_;
  } else {
    hard_fault_delta = previous_observation_ - hard_fault_count;
  }
  previous_observation_ = hard_fault_count;
  swap_thrashing_observation_window_.AddSample(hard_fault_delta);
  swap_thrashing_cooldown_window_.AddSample(hard_fault_delta);

  switch (previous_state) {
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE: {
      if (swap_thrashing_observation_window_.IsReady() &&
          swap_thrashing_observation_window_.GetAverage() >
              kThrashSwappingPageFaultThreshold) {
        LOG(ERROR) << "Switch to suspected.";
        swap_thrashing_observation_window_.ResetWindow(
            kSuspectedToConfirmedSampleCount);
        swap_thrashing_cooldown_window_.ResetWindow(
            kSuspectedIntervalCoolDownSampleCount);
        return SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED: {
      if (swap_thrashing_observation_window_.IsReady() &&
          swap_thrashing_observation_window_.GetAverage() >
              kThrashSwappingPageFaultThreshold) {
        LOG(ERROR) << "Switch to Confirmed.";
        swap_thrashing_observation_window_.ResetWindow(0U);
        swap_thrashing_cooldown_window_.ResetWindow(
            kConfirmedCoolDownSampleCount);
        return SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED;
      } else if (swap_thrashing_cooldown_window_.IsReady() &&
                 swap_thrashing_cooldown_window_.GetAverage() <
                     kPageFaultCooldownThreshold) {
        LOG(ERROR) << "Switch to None.";
        swap_thrashing_observation_window_.ResetWindow(
            kNoneToSuspectedSampleCount);
        swap_thrashing_cooldown_window_.ResetWindow(0U);
        return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED: {
      if (swap_thrashing_cooldown_window_.IsReady() &&
          swap_thrashing_cooldown_window_.GetAverage() <
              kPageFaultCooldownThreshold) {
        LOG(ERROR) << "Switch to Suspected from confirmed.";
        swap_thrashing_observation_window_.ResetWindow(
            kSuspectedToConfirmedSampleCount);
        swap_thrashing_cooldown_window_.ResetWindow(
            kSuspectedIntervalCoolDownSampleCount);
        return SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED;
      }
      break;
    }
    default:
      NOTREACHED();
  }

  return previous_state;
}

}  // namespace base
