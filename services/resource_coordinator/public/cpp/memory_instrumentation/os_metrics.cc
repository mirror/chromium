// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/os_metrics.h"

#include "base/format_macros.h"
#include "base/strings/stringprintf.h"

namespace memory_instrumentation {

// static
bool OSMetrics::FillProcessMemoryMaps(base::ProcessId pid,
                                      mojom::RawOSMemDump* dump) {
  auto maps = GetProcessMemoryMaps(pid);
  if (maps.empty())
    return false;

  dump->memory_maps = std::move(maps);

  return true;
}

// static
void OSMetrics::MemoryMapsAsValueInto(
    const std::vector<mojom::VmRegionPtr>& memory_maps,
    base::trace_event::TracedValue* value,
    bool is_argument_filtering_enabled) {
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

    // The module path will be the basename when argument filtering is
    // activated. The whitelisting implemented for filtering string values
    // doesn't allow rewriting. Therefore, a different path is produced here
    // when argument filtering is activated.
    if (is_argument_filtering_enabled) {
      base::FilePath::StringType module_path(region->mapped_file.begin(),
                                             region->mapped_file.end());
      value->SetString("mf",
                       base::FilePath(module_path).BaseName().AsUTF8Unsafe());
    } else {
      value->SetString("mf", region->mapped_file);
    }

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

}  // namespace memory_instrumentation
