// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/debug/stack_trace.h"

#include <link.h>
#include <magenta/crashlogger.h>
#include <magenta/process.h>
#include <magenta/syscalls.h>
#include <magenta/syscalls/definitions.h>
#include <magenta/syscalls/port.h>
#include <magenta/types.h>
#include <stddef.h>
#include <string.h>
#include <threads.h>
#include <unwind.h>

#include <algorithm>
#include <iomanip>
#include <iostream>

#include "base/logging.h"

#define rdebug_off_lmap offsetof(struct r_debug, r_map)

#define lmap_off_next offsetof(struct link_map, l_next)
#define lmap_off_name offsetof(struct link_map, l_name)
#define lmap_off_addr offsetof(struct link_map, l_addr)

namespace base {
namespace debug {

namespace {

const char kProcessNamePrefix[] = "app:";
const size_t kProcessNamePrefixLen = arraysize(kProcessNamePrefix) - 1;

struct BacktraceData {
  void** trace_array;
  size_t* count;
  size_t max;
};

_Unwind_Reason_Code UnwindStore(struct _Unwind_Context* context,
                                void* user_data) {
  BacktraceData* data = reinterpret_cast<BacktraceData*>(user_data);
  uintptr_t pc = _Unwind_GetIP(context);
  data->trace_array[*data->count] = reinterpret_cast<void*>(pc);
  *data->count += 1;
  if (*data->count == data->max)
    return _URC_END_OF_STACK;
  return _URC_NO_REASON;
}

// Stores and queries debugging symbol map info for the current process.
class SymbolMap {
 public:
  struct Entry {
    void* addr;
    char name[MX_MAX_NAME_LEN];

    bool operator<(const Entry& other) const { return addr >= other.addr; }
  };

  SymbolMap();
  ~SymbolMap() = default;

  // Gets the symbol map entry for |address|. Returns null if no entry could be
  // found for the address, or if the symbol map could not be queried.
  Entry* GetForAddress(void* address);

 private:
  static const size_t kMaxMapEntries = 64;

  void Populate();

  // Sorted in descending order by address, for lookup purposes.
  Entry entries_[kMaxMapEntries];

  size_t count_ = 0;
  bool valid_ = false;

  DISALLOW_COPY_AND_ASSIGN(SymbolMap);
};

SymbolMap::SymbolMap() {
  Populate();
}

SymbolMap::Entry* SymbolMap::GetForAddress(void* address) {
  if (!valid_) {
    return nullptr;
  }

  // Working backwards in the address space, return the first map entry whose
  // address comes before |address| (thereby enclosing it.)
  for (size_t i = 0; i < count_; ++i) {
    if (address >= entries_[i].addr) {
      return &entries_[i];
    }
  }
  return nullptr;
}

void SymbolMap::Populate() {
  mx_handle_t process = mx_process_self();

  // Get the process' name.
  char app_name[MX_MAX_NAME_LEN + kProcessNamePrefixLen];
  strcpy(app_name, kProcessNamePrefix);
  auto status = mx_object_get_property(
      process, MX_PROP_NAME, app_name + kProcessNamePrefixLen,
      sizeof(app_name) - kProcessNamePrefixLen);
  if (status != MX_OK) {
    DPLOG(WARNING)
        << "Couldn't get name, falling back to 'app' for program name: "
        << status;
    strlcpy(app_name, "app", sizeof(app_name));
  }

  // Retrieve the debug info struct.
  constexpr int map_capacity = sizeof(entries_);
  uintptr_t debug_addr;
  status = mx_object_get_property(process, MX_PROP_PROCESS_DEBUG_ADDR,
                                  &debug_addr, sizeof(debug_addr));
  if (status != MX_OK) {
    DPLOG(ERROR) << "Couldn't get symbol map for process: " << status;
    return;
  }
  r_debug* debug_info = reinterpret_cast<r_debug*>(debug_addr);

  // Get the link map from the debug info struct.
  link_map* lmap = reinterpret_cast<link_map*>(debug_info->r_map);
  if (!lmap) {
    DPLOG(ERROR) << "Null link_map for process.";
    return;
  }

  // Copy the contents of the link map linked list to |entries_|.
  while (lmap != nullptr) {
    if (count_ == map_capacity) {
      break;
    }
    SymbolMap::Entry* next_entry = &entries_[count_];
    count_++;

    next_entry->addr = reinterpret_cast<void*>(lmap->l_addr);
    char* name_to_use = lmap->l_name[0] ? lmap->l_name : app_name;
    size_t name_len = strnlen(name_to_use, MX_MAX_NAME_LEN);
    strncpy(next_entry->name, name_to_use, name_len);
    lmap = lmap->l_next;
  }

  std::sort(&entries_[0], &entries_[count_ - 1]);

  valid_ = true;
}

}  // namespace

// static
bool EnableInProcessStackDumping() {
  // StackTrace works to capture the current stack (e.g. for diagnostics added
  // to code), but for local capture and print of backtraces, we just let the
  // system crashlogger take over. It handles printing out a nicely formatted
  // backtrace with dso information, relative offsets, etc. that we can then
  // filter with addr2line in the run script to get file/line info.
  return true;
}

StackTrace::StackTrace(size_t count) : count_(0) {
  BacktraceData data = {&trace_[0], &count_,
                        std::min(count, static_cast<size_t>(kMaxTraces))};
  _Unwind_Backtrace(&UnwindStore, &data);
}

void StackTrace::Print() const {
  OutputToStream(&std::cerr);
}

void StackTrace::OutputToStream(std::ostream* os) const {
  SymbolMap map;

  size_t i = 0;
  for (; (i < count_) && os->good(); ++i) {
    auto entry = map.GetForAddress(trace_[i]);
    if (entry) {
      size_t offset = reinterpret_cast<uintptr_t>(trace_[i]) -
                      reinterpret_cast<uintptr_t>(entry->addr);
      (*os) << "bt#" << std::setfill('0') << std::setw(2) << i << ": pc "
            << trace_[i] << " (" << entry->name << ",0x" << std::hex << offset
            << ")\n";
    } else {
      // If debug symbols aren't available for this PC.
      (*os) << "bt#" << std::setfill('0') << std::setw(2) << i << ": pc "
            << trace_[i] << " (?)\n";
    }
  }

  (*os) << "bt#" << std::setfill('0') << std::setw(2) << i << ": end\n";
}

}  // namespace debug
}  // namespace base
