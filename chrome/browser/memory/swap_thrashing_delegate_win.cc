// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/swap_thrashing_delegate_win.h"

#include <windows.h>
#include <winternl.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_constants.h"

namespace memory {

namespace {

using SwapThrashingLevel = SwapThrashingMonitorDelegateWin::SwapThrashingLevel;

// The signature of the NtQuerySystemInformation function.
typedef NTSTATUS(WINAPI* NtQuerySystemInformationPtr)(SYSTEM_INFORMATION_CLASS,
                                                      PVOID,
                                                      ULONG,
                                                      PULONG);

// The period of time during which swap-thrashing has to be observed in order
// to switch from the SWAP_THRASHING_LEVEL_NONE to the
// SWAP_THRASHING_LEVEL_SUSPECTED level.
// TODO(sebmarchand): Confirm that this value is appropriate.
constexpr base::TimeDelta kNoneToSuspectedInterval =
    base::TimeDelta::FromMilliseconds(6000);

// The period of time during which swap-thrashing should not be observed in
// order to switch from the SWAP_THRASHING_LEVEL_SUSPECTED to the
// SWAP_THRASHING_LEVEL_NONE level.
// TODO(sebmarchand): Confirm that this value is appropriate.
constexpr base::TimeDelta kSuspectedToNoneInterval =
    base::TimeDelta::FromMilliseconds(8000);

// The period of time during which swap-thrashing has to be observed while
// being in the SWAP_THRASHING_LEVEL_SUSPECTED state in order to switch to the
// SWAP_THRASHING_LEVEL_CONFIRMED level.
// TODO(sebmarchand): Confirm that this value is appropriate.
constexpr base::TimeDelta kSuspectedToConfirmedInterval =
    base::TimeDelta::FromMilliseconds(10000);

// The period of time during which swap-thrashing should not be observed in
// order to switch from the SWAP_THRASHING_LEVEL_CONFIRMED to the
// SWAP_THRASHING_LEVEL_SUSPECTED level.
// TODO(sebmarchand): Confirm that this value is appropriate.
constexpr base::TimeDelta kConfirmedToSuspectedInterval =
    base::TimeDelta::FromMilliseconds(16000);

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
    if (base::FilePath::CompareEqualIgnoreCase(
            proc_info->ImageName.Buffer,
            chrome::kBrowserProcessExecutableName)) {
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

const size_t SwapThrashingMonitorDelegateWin::kPageFaultEscalationThreshold = 5;
const size_t SwapThrashingMonitorDelegateWin::kPageFaultCooldownThreshold = 3;

SwapThrashingMonitorDelegateWin::SwapThrashingMonitorDelegateWin()
    : last_thrashing_state_(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {
  // Initialize the observation windows. Initially we can only transition to the
  // suspected state.
  ResetEscalationWindow(kNoneToSuspectedInterval);
  swap_thrashing_cooldown_window_.reset(nullptr);
  // Do the initial observations.
  GetChromeHardPageFaultCounter();
}

SwapThrashingMonitorDelegateWin::~SwapThrashingMonitorDelegateWin() {}

SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
    PageFaultDetectionWindow(base::TimeDelta window_length)
    : window_length_(window_length) {
  Reset(window_length);
}

SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
    ~PageFaultDetectionWindow() {}

void SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::Reset(
    base::TimeDelta window_length) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observations_.clear();
  window_length_ = window_length;
}

void SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::OnObservation(
    PageFaultObservation observation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observations_.push_back(observation);

  // No need to remove any observation if there's only 1 or 2 of them.
  if (observations_.size() <= 2)
    return;

  // Find the oldest observation that we should keep.

  const auto& last_observation = observations_.back();
  // Start by looking at the second to last observation.
  auto last_iterator_to_keep = observations_.end();
  std::advance(last_iterator_to_keep, -2);

  // Look for the most recent iteration that gives us a full coverage of this
  // time window, if none is found then |last_iterator_to_keep| will point to
  // the first observation and none will be removed.
  while (last_iterator_to_keep != observations_.begin()) {
    if ((last_observation.timestamp - last_iterator_to_keep->timestamp) >=
        window_length_) {
      break;
    }
    last_iterator_to_keep--;
  }

  observations_.erase(observations_.begin(), last_iterator_to_keep);

  DCHECK_GE(observations_.size(), 2U);
}

bool SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
    AveragePageFaultRate(double* average_rate) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(average_rate != nullptr);
  if (observations_.size() < 2)
    return false;
  base::TimeDelta observation_length =
      observations_.back().timestamp - observations_.begin()->timestamp;
  if (observation_length < window_length_)
    return false;

  *average_rate = (observations_.back().page_fault_count -
                   observations_.begin()->page_fault_count) /
                  observation_length.InSecondsF();

  return true;
}

SwapThrashingLevel
SwapThrashingMonitorDelegateWin::CalculateCurrentSwapThrashingLevel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!GetChromeHardPageFaultCounter())
    return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;

  bool transition_to_higher_state = false;
  // Check if the observation window has expired and if there should be a state
  // change.
  if (swap_thrashing_escalation_window_) {
    double average_rate = 0;
    if (swap_thrashing_escalation_window_->AveragePageFaultRate(
            &average_rate)) {
      transition_to_higher_state =
          average_rate >= kPageFaultEscalationThreshold;
    }
  }
  bool transition_to_lower_state = false;
  // Check if the cooldown window has expired and if there should be a state
  // change.
  if (swap_thrashing_cooldown_window_) {
    double average_rate = 0;
    if (swap_thrashing_cooldown_window_->AveragePageFaultRate(&average_rate)) {
      transition_to_lower_state = average_rate <= kPageFaultCooldownThreshold;
    }
  }

  switch (last_thrashing_state_) {
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE: {
      if (transition_to_higher_state) {
        ResetEscalationWindow(kSuspectedToConfirmedInterval);
        ResetCooldownWindow(kSuspectedToNoneInterval);
        last_thrashing_state_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED: {
      if (transition_to_higher_state) {
        swap_thrashing_escalation_window_.reset(nullptr);
        ResetCooldownWindow(kConfirmedToSuspectedInterval);
        last_thrashing_state_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED;
      } else if (transition_to_lower_state) {
        ResetEscalationWindow(kNoneToSuspectedInterval);
        swap_thrashing_cooldown_window_.reset(nullptr);
        last_thrashing_state_ = SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED: {
      if (transition_to_lower_state) {
        ResetEscalationWindow(kSuspectedToConfirmedInterval);
        ResetCooldownWindow(kSuspectedToNoneInterval);
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
  PageFaultObservation observation = {hard_fault_count, base::TimeTicks::Now()};

  if (swap_thrashing_escalation_window_) {
    swap_thrashing_escalation_window_->OnObservation(observation);
  }
  if (swap_thrashing_cooldown_window_) {
    swap_thrashing_cooldown_window_->OnObservation(observation);
  }

  return true;
}

void SwapThrashingMonitorDelegateWin::ResetEscalationWindow(
    base::TimeDelta window_length) {
  if (!swap_thrashing_escalation_window_) {
    swap_thrashing_escalation_window_ =
        base::MakeUnique<PageFaultDetectionWindow>(window_length);
  } else {
    PageFaultObservation recent_observation =
        swap_thrashing_escalation_window_->MostRecentObservation();
    swap_thrashing_escalation_window_->Reset(window_length);
    swap_thrashing_escalation_window_->OnObservation(recent_observation);
  }
}

void SwapThrashingMonitorDelegateWin::ResetCooldownWindow(
    base::TimeDelta window_length) {
  if (!swap_thrashing_cooldown_window_) {
    swap_thrashing_cooldown_window_ =
        base::MakeUnique<PageFaultDetectionWindow>(window_length);
  } else {
    PageFaultObservation recent_observation =
        swap_thrashing_cooldown_window_->MostRecentObservation();
    swap_thrashing_cooldown_window_->Reset(window_length);
    swap_thrashing_cooldown_window_->OnObservation(recent_observation);
  }
}

}  // namespace memory
