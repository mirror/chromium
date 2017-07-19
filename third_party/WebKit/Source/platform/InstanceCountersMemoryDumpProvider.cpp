// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/InstanceCountersMemoryDumpProvider.h"

#include "base/trace_event/process_memory_dump.h"
#include "platform/InstanceCounters.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

const char kInstanceCountersDumpName[] = "counters";

InstanceCountersMemoryDumpProvider*
InstanceCountersMemoryDumpProvider::Instance() {
  DEFINE_STATIC_LOCAL(InstanceCountersMemoryDumpProvider, instance, ());
  return &instance;
}

bool InstanceCountersMemoryDumpProvider::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* memory_dump) {
  using base::trace_event::MemoryAllocatorDump;
  MemoryAllocatorDump* counter_dump =
      memory_dump->CreateAllocatorDump(kInstanceCountersDumpName);
  counter_dump->AddScalar(
      "audio_handler", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kAudioHandlerCounter));
  counter_dump->AddScalar(
      "document", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kDocumentCounter));
  counter_dump->AddScalar(
      "frame", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kFrameCounter));
  counter_dump->AddScalar("js_event_listener",
                          MemoryAllocatorDump::kUnitsObjects,
                          InstanceCounters::CounterValue(
                              InstanceCounters::kJSEventListenerCounter));
  counter_dump->AddScalar(
      "layout_object", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kLayoutObjectCounter));
  counter_dump->AddScalar("media_key_session",
                          MemoryAllocatorDump::kUnitsObjects,
                          InstanceCounters::CounterValue(
                              InstanceCounters::kMediaKeySessionCounter));
  counter_dump->AddScalar(
      "media_keys", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kMediaKeysCounter));
  counter_dump->AddScalar(
      "node", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kNodeCounter));
  counter_dump->AddScalar(
      "resource", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kResourceCounter));
  counter_dump->AddScalar(
      "script_promise", MemoryAllocatorDump::kUnitsObjects,
      InstanceCounters::CounterValue(InstanceCounters::kScriptPromiseCounter));
  counter_dump->AddScalar("suspendable_object",
                          MemoryAllocatorDump::kUnitsObjects,
                          InstanceCounters::CounterValue(
                              InstanceCounters::kSuspendableObjectCounter));
  counter_dump->AddScalar("v8_per_context_data",
                          MemoryAllocatorDump::kUnitsObjects,
                          InstanceCounters::CounterValue(
                              InstanceCounters::kV8PerContextDataCounter));
  counter_dump->AddScalar("worker_global_scope",
                          MemoryAllocatorDump::kUnitsObjects,
                          InstanceCounters::CounterValue(
                              InstanceCounters::kWorkerGlobalScopeCounter));

  return true;
}

}  // namespace blink
