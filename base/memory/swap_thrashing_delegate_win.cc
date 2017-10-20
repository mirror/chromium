// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_delegate_win.h"

#include <windows.h>
#include <winternl.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/win/win_util.h"

namespace base {

namespace {

using SwapThrashingLevel = SwapThrashingMonitorDelegateWin::SwapThrashingLevel;

// The signature of the NtQuerySystemInformation function.
typedef NTSTATUS(WINAPI* NtQuerySystemInformationPtr)(SYSTEM_INFORMATION_CLASS,
                                                      PVOID,
                                                      ULONG,
                                                      PULONG);

// TODO(sebmarchand): Switch to constexpr base::TimeDelta for these values.

// The period of time during which swap-thrashing has to be observed in order
// to switch from the SWAP_THRASHING_LEVEL_NONE to the
// SWAP_THRASHING_LEVEL_SUSPECTED level, in milliseconds.
// TODO(sebmarchand): Confirm that this value is appropriate.
const int kNoneToSuspectedIntervalMs = 6000;

// The period of time during which swap-thrashing should not be observed in
// order to switch from the SWAP_THRASHING_LEVEL_SUSPECTED to the
// SWAP_THRASHING_LEVEL_NONE level, in milliseconds.
// TODO(sebmarchand): Confirm that this value is appropriate.
const int kSuspectedIntervalCoolDownMs = 8000;

// The period of time during which swap-thrashing has to be observed while
// being in the SWAP_THRASHING_LEVEL_SUSPECTED state in order to switch to the
// SWAP_THRASHING_LEVEL_CONFIRMED level, in milliseconds.
// TODO(sebmarchand): Confirm that this value is appropriate.
const int kSuspectedToConfirmedIntervalMs = 10000;

// The period of time during which swap-thrashing should not be observed in
// order to switch from the SWAP_THRASHING_LEVEL_CONFIRMED to the
// SWAP_THRASHING_LEVEL_SUSPECTED level, in milliseconds.
// TODO(sebmarchand): Confirm that this value is appropriate.
const int kConfirmedCoolDownMs = 16000;

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

// The average hard page-fault rate (/sec) that we expect to see when switching
// to a higher swap-thrashing level.
// TODO(sebmarchand): Confirm that this value is appropriate.
const size_t
    SwapThrashingMonitorDelegateWin::kThrashSwappingPageFaultThreshold = 5;

// The average hard page-fault rate (/sec) that we use to go down to a lower
// swap-thrashing level.
// TODO(sebmarchand): Confirm that this value is appropriate.
const size_t SwapThrashingMonitorDelegateWin::kPageFaultCooldownThreshold = 3;

SwapThrashingMonitorDelegateWin::SwapThrashingMonitorDelegateWin()
    : last_thrashing_state_(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {
  GetChromeHardPageFaultCounter();
  swap_thrashing_detection_window_ = base::MakeUnique<PageFaultDetectionWindow>(
      kNoneToSuspectedIntervalMs, previous_observation_);
  swap_thrashing_cooldown_window_.reset(nullptr);
}

SwapThrashingMonitorDelegateWin::~SwapThrashingMonitorDelegateWin() {}

SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
    PageFaultDetectionWindow(int window_length_ms, uint64_t page_fault_count)
    : window_length_ms_(window_length_ms),
      initial_page_fault_count_(page_fault_count),
      window_beginning_(Time::Now()),
      average_page_fault_rate_(-1) {}

SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
    ~PageFaultDetectionWindow() {}

void SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::OnObservation(
    uint64_t page_fault_observation,
    Time observation_timestamp) {
  // No need to record this observation if the window hasn't expired.
  if (!IsReady())
    return;
  double time_elapsed_sec =
      static_cast<double>(
          (observation_timestamp - window_beginning_).InMilliseconds()) /
      Time::kMillisecondsPerSecond;
  average_page_fault_rate_ =
      (page_fault_observation - initial_page_fault_count_) / time_elapsed_sec;
}

bool SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::IsReady()
    const {
  if ((Time::Now() - window_beginning_).InMilliseconds() >= window_length_ms_)
    return true;
  return false;
}

SwapThrashingLevel
SwapThrashingMonitorDelegateWin::CalculateCurrentSwapThrashingLevel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!GetChromeHardPageFaultCounter())
    return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;

  if (swap_thrashing_detection_window_) {
    swap_thrashing_detection_window_->OnObservation(
        previous_observation_, previous_observation_timestamp_);
  }
  if (swap_thrashing_cooldown_window_) {
    swap_thrashing_cooldown_window_->OnObservation(
        previous_observation_, previous_observation_timestamp_);
  }

  bool transition_to_higher_state =
      swap_thrashing_detection_window_ &&
      swap_thrashing_detection_window_->IsReady() &&
      (swap_thrashing_detection_window_->average_page_fault_rate() >=
       kThrashSwappingPageFaultThreshold);
  bool transition_to_lower_state =
      swap_thrashing_cooldown_window_ &&
      swap_thrashing_cooldown_window_->IsReady() &&
      (swap_thrashing_cooldown_window_->average_page_fault_rate() <=
       kPageFaultCooldownThreshold);

  switch (last_thrashing_state_) {
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE: {
      if (transition_to_higher_state) {
        ResetDetectionWindow(kSuspectedToConfirmedIntervalMs,
                             previous_observation_);
        ResetCooldownWindow(kSuspectedIntervalCoolDownMs,
                            previous_observation_);
        last_thrashing_state_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED: {
      if (transition_to_higher_state) {
        swap_thrashing_detection_window_.reset(nullptr);
        ResetCooldownWindow(kConfirmedCoolDownMs, previous_observation_);
        last_thrashing_state_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED;
      } else if (transition_to_lower_state) {
        LOG(ERROR) << "Switch to None.";
        ResetDetectionWindow(kNoneToSuspectedIntervalMs, previous_observation_);
        swap_thrashing_cooldown_window_.reset(nullptr);
        last_thrashing_state_ = SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED: {
      if (transition_to_lower_state) {
        ResetDetectionWindow(kSuspectedToConfirmedIntervalMs,
                             previous_observation_);
        ResetCooldownWindow(kSuspectedIntervalCoolDownMs,
                            previous_observation_);
        last_thrashing_state_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED;
      }
      break;
    }
    default:
      NOTREACHED();
  }

  return last_thrashing_state_;
}

bool SwapThrashingMonitorDelegateWin::GetChromeHardPageFaultCounter() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  uint64_t hard_fault_count = 0;
  if (!GetHardFaultCountForChromeProcesses(&hard_fault_count)) {
    LOG(ERROR) << "Unable to retrieve the hard fault counts for Chrome "
               << "processes.";
    return false;
  }
  previous_observation_ = hard_fault_count;
  previous_observation_timestamp_ = Time::Now();
  return true;
}

void SwapThrashingMonitorDelegateWin::ResetDetectionWindow(
    int window_length_ms,
    uint64_t page_fault_count) {
  swap_thrashing_detection_window_ = base::MakeUnique<PageFaultDetectionWindow>(
      window_length_ms, page_fault_count);
}

void SwapThrashingMonitorDelegateWin::ResetCooldownWindow(
    int window_length_ms,
    uint64_t page_fault_count) {
  swap_thrashing_cooldown_window_ = base::MakeUnique<PageFaultDetectionWindow>(
      window_length_ms, page_fault_count);
}

}  // namespace base
