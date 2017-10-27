// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/swap_thrashing_monitor_delegate_win.h"

#include <windows.h>
#include <winternl.h>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_constants.h"

namespace memory {

namespace {

// The signature of the NtQuerySystemInformation function.
typedef NTSTATUS(WINAPI* NtQuerySystemInformationPtr)(SYSTEM_INFORMATION_CLASS,
                                                      PVOID,
                                                      ULONG,
                                                      PULONG);

// The threshold that we use to consider that an hard page-fault delta sample is
// high.
const size_t kHighSwappingThreshold = 8;

// The number of sample that we'll keep track off.
const size_t kWindowSampleCount = 12;

// The minimum number of samples that need to be above the threshold to be in
// the suspected state.
const size_t kSuspectedStateSampleCount = 5;

// The minimum number of samples that need to be above the threshold to be in
// the confirmed state.
const size_t kConfirmedStateSampleCount = 10;

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
//
// TODO(sebmarchand): Ideally we should reduce code duplication by re-using the
// TaskManager's code to gather these metrics, but it's not as simple as it
// should. It'll either require a lot of duct tape code to connect these 2
// things together, or the QuerySystemProcessInformation function in
// chrome/browser/task_manager/sampling/shared_sampler_win.cc should be moved
// somewhere else to make it easier to re-use it without having to implement
// all the task manager specific stuff.
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
    }
    // The list ends when NextEntryOffset is zero. This also prevents busy
    // looping if the data is in fact invalid.
    if (proc_info->NextEntryOffset <= 0)
      break;
    index += proc_info->NextEntryOffset;
  }

  return true;
}

}  // namespace

SwapThrashingMonitorDelegateWin::SwapThrashingMonitorDelegateWin()
    : samples_window_(base::MakeUnique<SampleWindow>()) {}

SwapThrashingMonitorDelegateWin::~SwapThrashingMonitorDelegateWin() {}

SwapThrashingMonitorDelegateWin::SampleWindow::SampleWindow()
    : kSampleCount(kWindowSampleCount),
      last_observation_(),
      observation_abobe_threshold_(0U) {}

SwapThrashingMonitorDelegateWin::SampleWindow::~SampleWindow() {}

void SwapThrashingMonitorDelegateWin::SampleWindow::OnObservation(
    uint64_t sample) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (last_observation_ == base::nullopt) {
    last_observation_ = sample;
    return;
  }

  // Start by removing the sample that have expired.
  if (observation_deltas_.size() == kSampleCount) {
    if (observation_deltas_.front() >= kHighSwappingThreshold) {
      DCHECK_GT(observation_abobe_threshold_, 0U);
      --observation_abobe_threshold_;
    }
    observation_deltas_.pop_front();
  }

  size_t delta = sample - last_observation_.value();
  observation_deltas_.push_back(delta);
  last_observation_ = sample;
  if (delta >= kHighSwappingThreshold)
    ++observation_abobe_threshold_;
}

SwapThrashingLevel
SwapThrashingMonitorDelegateWin::SampleAndCalculateSwapThrashingLevel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!RecordHardFaultCountForChromeProcesses())
    return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;

  SwapThrashingLevel level = SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;

  size_t sample_above_threshold =
      samples_window_->observation_abobe_threshold();
  if (sample_above_threshold >= kConfirmedStateSampleCount) {
    level = SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED;
  } else if (sample_above_threshold >= kSuspectedStateSampleCount) {
    level = SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED;
  }

  return level;
}

bool SwapThrashingMonitorDelegateWin::RecordHardFaultCountForChromeProcesses() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  uint64_t hard_fault_count = 0;
  if (!GetHardFaultCountForChromeProcesses(&hard_fault_count)) {
    LOG(ERROR) << "Unable to retrieve the hard fault counts for Chrome "
               << "processes.";
    return false;
  }
  samples_window_->OnObservation(hard_fault_count);

  return true;
}

}  // namespace memory
