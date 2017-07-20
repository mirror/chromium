// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/tracing_observer.h"

#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_event_argument.h"

namespace memory_instrumentation {

using base::trace_event::TracedValue;
using base::trace_event::ProcessMemoryDump;

namespace {

const int kTraceEventNumArgs = 1;
const char* kTraceEventArgNames[] = {"dumps"};
const unsigned char kTraceEventArgTypes[] = {TRACE_VALUE_TYPE_CONVERTABLE};

bool IsMemoryInfraTracingEnabled() {
  bool enabled;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(
      base::trace_event::MemoryDumpManager::kTraceCategory, &enabled);
  return enabled;
}

void OsDumpAsValueInto(TracedValue* value, const mojom::OSMemDump& os_dump) {
  value->SetString(
      "resident_set_bytes",
      base::StringPrintf("%" PRIx32, os_dump.resident_set_kb * 1024));
  value->SetString(
      "private_footprint_bytes",
      base::StringPrintf("%" PRIx32, os_dump.private_footprint_kb * 1024));
}

void MemoryMapsAsValueInto(TracedValue* value,
                           const std::vector<mojom::VmRegionPtr>& memory_maps) {
  static const char kHexFmt[] = "%" PRIx64;

  // Refer to the design doc goo.gl/sxfFY8 for the semantics of these fields.
  value->BeginArray("vm_regions");
  for (const auto& region : memory_maps) {
    value->BeginDictionary();

    value->SetString("sa", base::StringPrintf(kHexFmt, region->start_address));
    value->SetString("sz", base::StringPrintf(kHexFmt, region->size_in_bytes));
    if (region->module_timestamp)
      value->SetString("ts",
                       base::StringPrintf(kHexFmt, region->module_timestamp));
    value->SetInteger("pf", region->protection_flags);
    value->SetString("mf", region->mapped_file);

    value->BeginDictionary("bs");  // byte stats
    value->SetString(
        "pss",
        base::StringPrintf(kHexFmt, region->byte_stats_proportional_resident));
    value->SetString(
        "pd",
        base::StringPrintf(kHexFmt, region->byte_stats_private_dirty_resident));
    value->SetString(
        "pc",
        base::StringPrintf(kHexFmt, region->byte_stats_private_clean_resident));
    value->SetString(
        "sd",
        base::StringPrintf(kHexFmt, region->byte_stats_shared_dirty_resident));
    value->SetString(
        "sc",
        base::StringPrintf(kHexFmt, region->byte_stats_shared_clean_resident));
    value->SetString("sw",
                     base::StringPrintf(kHexFmt, region->byte_stats_swapped));
    value->EndDictionary();

    value->EndDictionary();
  }
  value->EndArray();
}

};  // namespace

TracingObserver::TracingObserver(
    base::trace_event::TraceLog* trace_log,
    base::trace_event::MemoryDumpManager* memory_dump_manager)
    : memory_dump_manager_(memory_dump_manager), trace_log_(trace_log) {
  // If tracing was enabled before initializing MemoryDumpManager, we missed the
  // OnTraceLogEnabled() event. Synthesize it so we can late-join the party.
  // IsEnabled is called before adding observer to avoid calling
  // OnTraceLogEnabled twice.
  bool is_tracing_already_enabled = trace_log_->IsEnabled();
  trace_log_->AddEnabledStateObserver(this);
  if (is_tracing_already_enabled)
    OnTraceLogEnabled();
}

TracingObserver::~TracingObserver() {
  trace_log_->RemoveEnabledStateObserver(this);
}

void TracingObserver::OnTraceLogEnabled() {
  if (!IsMemoryInfraTracingEnabled())
    return;

  // Initialize the TraceLog for the current thread. This is to avoids that the
  // TraceLog memory dump provider is registered lazily during the MDM
  // SetupForTracing().
  base::trace_event::TraceLog::GetInstance()
      ->InitializeThreadLocalEventBufferIfSupported();

  const base::trace_event::TraceConfig& trace_config =
      base::trace_event::TraceLog::GetInstance()->GetCurrentTraceConfig();
  const base::trace_event::TraceConfig::MemoryDumpConfig& memory_dump_config =
      trace_config.memory_dump_config();

  memory_dump_config_ =
      base::MakeUnique<base::trace_event::TraceConfig::MemoryDumpConfig>(
          memory_dump_config);

  if (memory_dump_manager_)
    memory_dump_manager_->SetupForTracing(memory_dump_config);
}

void TracingObserver::OnTraceLogDisabled() {
  if (memory_dump_manager_)
    memory_dump_manager_->TeardownForTracing();
  memory_dump_config_.reset();
}

bool TracingObserver::AddToTraceIfEnabled(
    const base::trace_event::MemoryDumpRequestArgs& args,
    const base::ProcessId pid,
    base::Callback<void(TracedValue*)> add_to_trace) {
  // If tracing has been disabled early out to avoid the cost of serializing the
  // dump then ignoring the result.
  if (!IsMemoryInfraTracingEnabled())
    return false;
  // If the dump mode is too detailed don't add to trace to avoid accidentally
  // including PII.
  if (!IsDumpModeAllowed(args.level_of_detail))
    return false;

  CHECK_NE(base::trace_event::MemoryDumpType::SUMMARY_ONLY, args.dump_type);

  std::unique_ptr<TracedValue> traced_value = base::MakeUnique<TracedValue>();

  add_to_trace.Run(traced_value.get());

  traced_value->SetString(
      "level_of_detail",
      base::trace_event::MemoryDumpLevelOfDetailToString(args.level_of_detail));
  const uint64_t dump_guid = args.dump_guid;
  const char* const event_name =
      base::trace_event::MemoryDumpTypeToString(args.dump_type);
  std::unique_ptr<base::trace_event::ConvertableToTraceFormat> event_value(
      std::move(traced_value));
  TRACE_EVENT_API_ADD_TRACE_EVENT_WITH_PROCESS_ID(
      TRACE_EVENT_PHASE_MEMORY_DUMP,
      base::trace_event::TraceLog::GetCategoryGroupEnabled(
          base::trace_event::MemoryDumpManager::kTraceCategory),
      event_name, trace_event_internal::kGlobalScope, dump_guid, pid,
      kTraceEventNumArgs, kTraceEventArgNames, kTraceEventArgTypes,
      nullptr /* arg_values */, &event_value, TRACE_EVENT_FLAG_HAS_ID);

  return true;
}

bool TracingObserver::AddDumpToTraceIfEnabled(
    const base::trace_event::MemoryDumpRequestArgs& req_args,
    const base::ProcessId pid,
    const ProcessMemoryDump* process_memory_dump) {
  return AddToTraceIfEnabled(
      req_args, pid,
      base::Bind(
          [](const ProcessMemoryDump* process_memory_dump, TracedValue* value) {
            process_memory_dump->AsValueInto(value);
          },
          process_memory_dump));
}

// void AddToTrace(
//    uint64_t dump_guid,
//    base::trace_event::MemoryDumpLevelOfDetail level_of_detail,
//    base::ProcessId pid,
//    const mojom::RawOSMemDump& os_dump) {
//
//  std::unique_ptr<base::trace_event::TracedValue> traced_value(
//      new base::trace_event::TracedValue);
//
//  //if () {
//  //  value->BeginDictionary("process_totals");
//  //  process_totals_.AsValueInto(value);
//  //  value->EndDictionary();
//  //}
//
//  if (os_dump.memory_maps.size()) {
//    value->BeginDictionary("process_mmaps");
//    MemoryMapsAsValueInto(traced_value.get(), os_dump.memory_maps);
//    value->EndDictionary();
//  }
//
//  traced_value->SetString("level_of_detail",
//                          base::trace_event::MemoryDumpLevelOfDetailToString(
//                              level_of_detail));
//  const char* const event_name =
//      base::trace_event::MemoryDumpTypeToString(req_args->dump_type);
//
//  std::unique_ptr<base::trace_event::ConvertableToTraceFormat> event_value(
//      std::move(traced_value));
//  TRACE_EVENT_API_ADD_TRACE_EVENT_WITH_PROCESS_ID(
//      TRACE_EVENT_PHASE_MEMORY_DUMP,
//      base::trace_event::TraceLog::GetCategoryGroupEnabled(
//          base::trace_event::MemoryDumpManager::kTraceCategory),
//      event_name, trace_event_internal::kGlobalScope, dump_guid, pid,
//      kTraceEventNumArgs, kTraceEventArgNames, kTraceEventArgTypes,
//      nullptr /* arg_values */, &event_value, TRACE_EVENT_FLAG_HAS_ID);
//}

bool TracingObserver::AddOsDumpToTraceIfEnabled(
    const base::trace_event::MemoryDumpRequestArgs& req_args,
    const base::ProcessId pid,
    const mojom::OSMemDump* os_dump,
    const std::vector<mojom::VmRegionPtr>* memory_maps) {
  return AddToTraceIfEnabled(
      req_args, pid,
      base::Bind(
          [](const mojom::OSMemDump* os_dump,
             const std::vector<mojom::VmRegionPtr>* memory_maps,
             TracedValue* value) {

            value->BeginDictionary("process_totals");
            OsDumpAsValueInto(value, *os_dump);
            value->EndDictionary();

            if (memory_maps->size()) {
              value->BeginDictionary("process_mmaps");
              MemoryMapsAsValueInto(value, *memory_maps);
              value->EndDictionary();
            }
          },
          os_dump, memory_maps));
}

bool TracingObserver::IsDumpModeAllowed(
    base::trace_event::MemoryDumpLevelOfDetail dump_mode) const {
  if (!memory_dump_config_)
    return false;
  return memory_dump_config_->allowed_dump_modes.count(dump_mode) != 0;
}

}  // namespace memory_instrumentation
