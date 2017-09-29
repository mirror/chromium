/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bindings/core/v8/V8Initializer.h"
#include "bindings/modules/v8/V8ContextSnapshotExternalReferences.h"
#include "build/build_config.h"
#include "core/animation/AnimationClock.h"
#include "modules/ModulesInitializer.h"
#include "platform/bindings/Microtask.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/heap/Heap.h"
#include "platform/wtf/Assertions.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/WTF.h"
#include "public/platform/Platform.h"
#include "public/platform/WebThread.h"
#include "public/web/WebKit.h"
#include "v8/include/v8.h"

namespace blink {

namespace {

class EndOfTaskRunner : public WebThread::TaskObserver {
 public:
  void WillProcessTask() override { AnimationClock::NotifyTaskStart(); }
  void DidProcessTask() override {
    Microtask::PerformCheckpoint(V8PerIsolateData::MainThreadIsolate());
    V8Initializer::ReportRejectedPromisesOnMainThread();
  }
};

#if defined(OS_WIN)
bool FindAndReleaseReservation(size_t size) {
  // See the SandboxedProcessLauncherDelegate::PostSpawnTarget method in
  // render_process_host_impl.cc to see how the memory is reserved. The memory
  // parameters must match what we use to find the reservation here.
  SYSTEM_INFO sys_info;
  GetSystemInfo(&sys_info);
  char* start = sys_info.lpMinimumApplicationAddress;
  while (true) {
    MEMORY_BASIC_INFORMATION mem;
    size_t mem_size = VirtualQuery((LPCVOID)start, &mem, sizeof(mem));
    if (mem_size == 0)
      break;
    CHECK(mem_size == sizeof(mem));

    if (mem.State == MEM_RESERVE && mem.AllocationProtect == PAGE_NOACCESS &&
        mem.RegionSize == size) {
      if (!VirtualFree(start, 0, MEM_RELEASE))
        break;
      return true;
    }
    start += mem.RegionSize;
    if ((LPVOID)start >= sys_info.lpMaximumApplicationAddress)
      break;
  }
  return false;
}
#endif  // defined(OS_WIN)

}  // namespace

static WebThread::TaskObserver* g_end_of_task_runner = nullptr;

static ModulesInitializer& GetModulesInitializer() {
  DEFINE_STATIC_LOCAL(std::unique_ptr<ModulesInitializer>, initializer,
                      (WTF::WrapUnique(new ModulesInitializer)));
  return *initializer;
}

void Initialize(Platform* platform) {
  Platform::Initialize(platform);

#if defined(OS_WIN)
  // Try to reserve a large region of address space on 32 bit Windows, to make
  // large array buffer allocations likelier to succeed.
  if (base::win::OSInfo::GetInstance()->wow64_status() !=
      base::win::OSInfo::WOW64_ENABLED) {
    const size_t kMB = 1024 * 1024;
    const size_t kMaxSize = 512 * kMB;
    const size_t kMinSize = 32 * kMB;
    const size_t kStepSize = 16 * kMB;
    // Search for a reserved block of 512 MB made on our behalf by the browser
    // and free it, so it's available for page allocator's reservation.
    if (!FindAndReleaseReservation(kMaxSize)) {
      DLOG(WARNING) << "Could not find and release reserved address space";
    }
    // Try to reserve as much address space as we reasonably can. We retry
    // since we might lose some or all of the reservation due to a race.
    for (size_t size = kMaxSize; size >= kMinSize; size -= kStepSize) {
      if (base::ReserveAddressSpace(size)) {
        break;
      }
    }
  }
#endif  // defined(OS_WIN)

  V8Initializer::InitializeMainThread(
      V8ContextSnapshotExternalReferences::GetTable());

  GetModulesInitializer().Initialize();

  // currentThread is null if we are running on a thread without a message loop.
  if (WebThread* current_thread = platform->CurrentThread()) {
    DCHECK(!g_end_of_task_runner);
    g_end_of_task_runner = new EndOfTaskRunner;
    current_thread->AddTaskObserver(g_end_of_task_runner);
  }
}

}  // namespace blink
