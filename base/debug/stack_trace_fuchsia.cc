// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/debug/stack_trace.h"

#include <link.h>
#include <magenta/process.h>
#include <magenta/syscalls.h>
#include <magenta/syscalls/port.h>
#include <magenta/types.h>
#include <ngunwind/libunwind.h>
#include <threads.h>
#include <unwind.h>

#include <algorithm>
#include <iostream>

#include "base/logging.h"

namespace base {
namespace debug {

namespace {

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

constexpr uint64_t kExceptionKey = 0x424144u;  // "BAD".
bool g_in_process_exception_handler_enabled;


#define rdebug_off_lmap offsetof(struct r_debug, r_map)

#define lmap_off_next offsetof(struct link_map, l_next)
#define lmap_off_name offsetof(struct link_map, l_name)
#define lmap_off_addr offsetof(struct link_map, l_addr)

typedef struct dsoinfo {
    struct dsoinfo* next;
    mx_vaddr_t base;
    bool debug_file_tried;
    mx_status_t debug_file_status;
    char* debug_file;
    char name[];
} dsoinfo_t;

mx_status_t read_mem(mx_handle_t h, mx_vaddr_t vaddr, void* ptr, size_t len) {
  size_t actual;
  mx_status_t status = mx_process_read_memory(h, vaddr, ptr, len, &actual);
  if (status < 0) {
    printf("read_mem @%p FAILED %zd\n", (void*)vaddr, len);
    return status;
  }
  if (len != actual) {
    printf("read_mem @%p FAILED, short read %zd\n", (void*)vaddr, len);
    return MX_ERR_IO;
  }
  return MX_OK;
}

mx_status_t fetch_string(mx_handle_t h,
                         mx_vaddr_t vaddr,
                         char* ptr,
                         size_t max) {
  while (max > 1) {
    mx_status_t status;
    if ((status = read_mem(h, vaddr, ptr, 1)) < 0) {
      *ptr = 0;
      return status;
    }
    ptr++;
    vaddr++;
    max--;
  }
  *ptr = 0;
  return MX_OK;
}

static dsoinfo_t* dsolist_add(dsoinfo_t** list,
                              const char* name,
                              uintptr_t base) {
  if (!strncmp(name, "app:devhost:", 12)) {
    // devhost processes use their name field to describe
    // the root of their device sub-tree.
    name = "app:/boot/bin/devhost";
  }
  size_t len = strlen(name);
  auto dso =
      reinterpret_cast<dsoinfo_t*>(calloc(1, sizeof(dsoinfo_t) + len + 1));
  if (dso == nullptr) {
    return nullptr;
  }
  memcpy(dso->name, name, len + 1);
  dso->base = base;
  dso->debug_file_tried = false;
  dso->debug_file_status = MX_ERR_BAD_STATE;
  while (*list != nullptr) {
    if ((*list)->base < dso->base) {
      dso->next = *list;
      *list = dso;
      return dso;
    }
    list = &((*list)->next);
  }
  *list = dso;
  dso->next = nullptr;
  return dso;
}

static dsoinfo_t* dso_fetch_list(mx_handle_t h, const char* name) {
  uintptr_t lmap, debug_addr;
  mx_status_t status = mx_object_get_property(h, MX_PROP_PROCESS_DEBUG_ADDR,
                                              &debug_addr, sizeof(debug_addr));
  if (status != MX_OK) {
    LOG(ERROR) << "mx_object_get_property(MX_PROP_PROCESS_DEBUG_ADDR), unable "
                  "to fetch dso list, status="
               << status;
    return nullptr;
  }
  if (read_mem(h, debug_addr + rdebug_off_lmap, &lmap, sizeof(lmap))) {
    return nullptr;
  }
  dsoinfo_t* dsolist = nullptr;
  while (lmap != 0) {
    char dsoname[64];
    mx_vaddr_t base;
    uintptr_t next;
    uintptr_t str;
    if (read_mem(h, lmap + lmap_off_addr, &base, sizeof(base))) {
      break;
    }
    if (read_mem(h, lmap + lmap_off_next, &next, sizeof(next))) {
      break;
    }
    if (read_mem(h, lmap + lmap_off_name, &str, sizeof(str))) {
      break;
    }
    if (fetch_string(h, str, dsoname, sizeof(dsoname))) {
      break;
    }
    dsolist_add(&dsolist, dsoname[0] ? dsoname : name, base);
    lmap = next;
  }

  return dsolist;
}

static void dso_free_list(dsoinfo_t* list) {
  while (list != NULL) {
    dsoinfo_t* next = list->next;
    free(list->debug_file);
    free(list);
    list = next;
  }
}

static dsoinfo_t* dso_lookup(dsoinfo_t* dso_list, mx_vaddr_t pc) {
  for (dsoinfo_t* dso = dso_list; dso != NULL; dso = dso->next) {
    if (pc >= dso->base) {
      return dso;
    }
  }

  return nullptr;
}

void dso_print_list(dsoinfo_t* dso_list) {
  for (dsoinfo_t* dso = dso_list; dso != nullptr; dso = dso->next) {
    printf("dso: base=%p name=%s\n", (void*)dso->base, dso->name);
  }
}

int SelfDumpFunc(void* arg) {
  mx_handle_t exception_port =
      static_cast<mx_handle_t>(reinterpret_cast<uintptr_t>(arg));

  mx_exception_packet_t packet;
  mx_status_t status =
      mx_port_wait(exception_port, MX_TIME_INFINITE, &packet, sizeof(packet));
  if (status < 0) {
    DLOG(ERROR) << "mx_port_wait failed: " << status;
    return 1;
  }
  if (packet.hdr.key != kExceptionKey) {
    DLOG(ERROR) << "unexpected crash key";
    return 1;
  }

  printf("Process crashed!\n");

#if 0
  mx_exception_context* context = &packet.report.context;

  unw_fuchsia_info_t* fuchsia =
      unw_create_fuchsia(process, thread, dsos, dso_lookup);
// data associated with an exception (siginfo in linux parlance)
typedef struct mx_exception_context {
    // One of ARCH_ID above.
    uint32_t arch_id;
    // The process of the thread with the exception.
    mx_koid_t pid;

    // The thread that got the exception.
    // This is zero in "process gone" notifications.
    mx_koid_t tid;

    struct {
        mx_vaddr_t pc;
        union {
            x86_64_exc_data_t x86_64;
            arm64_exc_data_t  arm_64;
        } u;
        // TODO(dje): add more stuff, revisit packing
        // For an example list of things one might add, see linux siginfo.
    } arch;
} mx_exception_context_t;
#endif

  StackTrace trace;
  size_t count;

  char name[MX_MAX_NAME_LEN];
  status = mx_object_get_property(mx_process_self(), MX_PROP_NAME, name,
                                  sizeof(name));
  if (status != MX_OK)
    strlcpy(name, "<unknown>", sizeof(name));

  dsoinfo_t* dso_info = dso_fetch_list(mx_process_self(), name);
  dso_print_list(dso_info);

  const void* const* address = trace.Addresses(&count);
  for (size_t i = 0; i < count; ++i) {
    mx_vaddr_t addr_as_vaddr = reinterpret_cast<mx_vaddr_t>(address[i]);
    dsoinfo_t* dso = dso_lookup(dso_info, addr_as_vaddr);
    uintptr_t offset = 0;
    if (dso)
      offset = addr_as_vaddr - dso->base;
    printf("  bt#%zi: pc %p (app:%s,0x%zx)\n", i, address[i], name, offset);
  }
  fflush(stdout);

  dso_free_list(dso_info);

  _exit(1);
}

bool SetInProcessExceptionHandler() {
  if (g_in_process_exception_handler_enabled)
    return true;

  mx_status_t status;
  mx_handle_t self_dump_port;
  status = mx_port_create(0u, &self_dump_port);
  if (status < 0) {
    DLOG(ERROR) << "mx_port_create failed: " << status;
    return false;
  }

  // A thread to wait for and process internal exceptions.
  thrd_t self_dump_thread;
  void* self_dump_arg =
      reinterpret_cast<void*>(static_cast<uintptr_t>(self_dump_port));
  int ret = thrd_create(&self_dump_thread, SelfDumpFunc, self_dump_arg);
  if (ret != thrd_success) {
    DLOG(ERROR) << "thrd_create failed: " << ret;
    return false;
  }

  status = mx_task_bind_exception_port(mx_process_self(), self_dump_port,
                                       kExceptionKey, 0);

  if (status < 0) {
    DLOG(ERROR) << "mx_task_bind_exception_port failed: " << status;
    return false;
  }

  g_in_process_exception_handler_enabled = true;
  return true;
}

}  // namespace

// static
bool EnableInProcessStackDumping() {
  return SetInProcessExceptionHandler();
}

StackTrace::StackTrace(size_t count) : count_(0) {
  BacktraceData data = {&trace_[0], &count_,
                        std::min(count, static_cast<size_t>(kMaxTraces))};
  _Unwind_Backtrace(&UnwindStore, &data);
}

void StackTrace::Print() const {
  OutputToStream(&std::cout);
}

void StackTrace::OutputToStream(std::ostream* os) const {
  // TODO(fuchsia): Consider doing symbol resolution here. See
  // https://crbug.com/706592.
  for (size_t i = 0; (i < count_) && os->good(); ++i) {
    (*os) << "\t" << trace_[i] << "\n";
  }
}

}  // namespace debug
}  // namespace base
