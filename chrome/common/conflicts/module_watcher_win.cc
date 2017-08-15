// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/conflicts/module_watcher_win.h"

#include <windows.h>

#include <tlhelp32.h>
#include <winternl.h>  // For UNICODE_STRING.

#include <string>
#include <utility>

#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/win/scoped_handle.h"

// These structures and functions are documented in MSDN, see
// http://msdn.microsoft.com/en-us/library/gg547638(v=vs.85).aspx
// there are however no headers or import libraries available in the
// Platform SDK. They are declared outside of the anonymous namespace to
// allow them to be forward declared in the header file.
enum {
  // The DLL was loaded. The NotificationData parameter points to a
  // LDR_DLL_LOADED_NOTIFICATION_DATA structure.
  LDR_DLL_NOTIFICATION_REASON_LOADED = 1,
  // The DLL was unloaded. The NotificationData parameter points to a
  // LDR_DLL_UNLOADED_NOTIFICATION_DATA structure.
  LDR_DLL_NOTIFICATION_REASON_UNLOADED = 2,
};

// Structure that is used for module load notifications.
struct LDR_DLL_LOADED_NOTIFICATION_DATA {
  // Reserved.
  ULONG Flags;
  // The full path name of the DLL module.
  PCUNICODE_STRING FullDllName;
  // The base file name of the DLL module.
  PCUNICODE_STRING BaseDllName;
  // A pointer to the base address for the DLL in memory.
  PVOID DllBase;
  // The size of the DLL image, in bytes.
  ULONG SizeOfImage;
};
using PLDR_DLL_LOADED_NOTIFICATION_DATA = LDR_DLL_LOADED_NOTIFICATION_DATA*;

// Structure that is used for module unload notifications.
struct LDR_DLL_UNLOADED_NOTIFICATION_DATA {
  // Reserved.
  ULONG Flags;
  // The full path name of the DLL module.
  PCUNICODE_STRING FullDllName;
  // The base file name of the DLL module.
  PCUNICODE_STRING BaseDllName;
  // A pointer to the base address for the DLL in memory.
  PVOID DllBase;
  // The size of the DLL image, in bytes.
  ULONG SizeOfImage;
};
using PLDR_DLL_UNLOADED_NOTIFICATION_DATA = LDR_DLL_UNLOADED_NOTIFICATION_DATA*;

// Union that is used for notifications.
union LDR_DLL_NOTIFICATION_DATA {
  LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
  LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
};
using PLDR_DLL_NOTIFICATION_DATA = LDR_DLL_NOTIFICATION_DATA*;

// Signature of the notification callback function.
using PLDR_DLL_NOTIFICATION_FUNCTION =
    VOID(CALLBACK*)(ULONG notification_reason,
                    const LDR_DLL_NOTIFICATION_DATA* notification_data,
                    PVOID context);

// Signatures of the functions used for registering DLL notification callbacks.
using LdrRegisterDllNotificationFunc =
    NTSTATUS(NTAPI*)(ULONG flags,
                     PLDR_DLL_NOTIFICATION_FUNCTION notification_function,
                     PVOID context,
                     PVOID* cookie);
using LdrUnregisterDllNotificationFunc = NTSTATUS(NTAPI*)(PVOID cookie);

namespace {

// Global lock for ensuring synchronization of destruction and notifications.
base::LazyInstance<base::Lock>::Leaky g_module_watcher_lock =
    LAZY_INSTANCE_INITIALIZER;
// Global pointer to the singleton ModuleWatcher, if one exists. Under
// |module_watcher_lock|.
ModuleWatcher* g_module_watcher_instance = nullptr;

// Names of the DLL notification registration functions. These are exported by
// ntdll.
constexpr wchar_t kNtDll[] = L"ntdll.dll";
constexpr char kLdrRegisterDllNotification[] = "LdrRegisterDllNotification";
constexpr char kLdrUnregisterDllNotification[] = "LdrUnregisterDllNotification";

// Helper function for converting a UNICODE_STRING to a FilePath.
base::FilePath ToFilePath(const UNICODE_STRING* str) {
  return base::FilePath(
      base::StringPiece16(str->Buffer, str->Length / sizeof(wchar_t)));
}

template <typename NotificationDataType>
ModuleWatcher::ModuleEvent CreateModuleEvent(
    mojom::ModuleEventType event_type,
    const NotificationDataType& notification_data) {
  return ModuleWatcher::ModuleEvent(
      event_type, ToFilePath(notification_data.FullDllName),
      notification_data.DllBase, notification_data.SizeOfImage);
}

// Returns a snapshot that contains all the modules in the current process.
base::win::ScopedHandle GetModuleListSnapshot() {
  base::win::ScopedHandle snapshot_handle;

  // According to MSDN, CreateToolhelp32Snapshot should be retried as long as
  // it's returning ERROR_BAD_LENGTH. To avoid locking up here a retry limit is
  // enforced.
  DWORD process_id = ::GetCurrentProcessId();
  for (size_t i = 0; i < 5; ++i) {
    snapshot_handle.Set(::CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id));
    if (snapshot_handle.IsValid() || ::GetLastError() != ERROR_BAD_LENGTH)
      break;
  }

  return snapshot_handle;
}

}  // namespace

// static
std::unique_ptr<ModuleWatcher> ModuleWatcher::Create(
    OnModuleEventCallback callback) {
  // If a ModuleWatcher already exists then bail out.
  base::AutoLock lock(g_module_watcher_lock.Get());
  if (g_module_watcher_instance)
    return nullptr;

  // This thread acquired the right to create a ModuleWatcher, so do so.
  g_module_watcher_instance = new ModuleWatcher(std::move(callback));

  return base::WrapUnique(g_module_watcher_instance);
}

void ModuleWatcher::Initialize() {
  RegisterDllNotificationCallback();
  EnumerateAlreadyLoadedModules();
}

ModuleWatcher::~ModuleWatcher() {
  // As soon as |g_module_watcher_instance| is null any dispatched callbacks
  // will be silently absorbed by LoaderNotificationCallback.
  base::AutoLock lock(g_module_watcher_lock.Get());
  DCHECK_EQ(g_module_watcher_instance, this);
  g_module_watcher_instance = nullptr;
  UnregisterDllNotificationCallback();
}

struct ModuleWatcher::ModuleEventCompare {
  bool operator()(const ModuleWatcher::ModuleEvent& lhs,
                  const ModuleWatcher::ModuleEvent& rhs) const {
    return std::tie(lhs.module_path, lhs.module_load_address, lhs.module_size) <
           std::tie(rhs.module_path, rhs.module_load_address, rhs.module_size);
  }
};

void ModuleWatcher::RegisterDllNotificationCallback() {
  register_dll_notification_callback_called_ = true;

  LdrRegisterDllNotificationFunc reg_fn =
      reinterpret_cast<LdrRegisterDllNotificationFunc>(::GetProcAddress(
          ::GetModuleHandle(kNtDll), kLdrRegisterDllNotification));
  if (reg_fn)
    reg_fn(0, &LoaderNotificationCallback, this, &dll_notification_cookie_);
}

void ModuleWatcher::UnregisterDllNotificationCallback() {
  LdrUnregisterDllNotificationFunc unreg_fn =
      reinterpret_cast<LdrUnregisterDllNotificationFunc>(::GetProcAddress(
          ::GetModuleHandle(kNtDll), kLdrUnregisterDllNotification));
  if (unreg_fn)
    unreg_fn(dll_notification_cookie_);
}

void ModuleWatcher::EnumerateAlreadyLoadedModules() {
  // Sanity check for unittests.
  DCHECK(register_dll_notification_callback_called_);

  base::win::ScopedHandle snapshot_handle = GetModuleListSnapshot();

  // To prevent a lock priority inversion, |g_module_watcher_lock| cannot be
  // held while calling into ::CreateToolhelp32Snapshot(). This does open up the
  // possibility of a race where it is not possible to determine if a dll
  // notification happened before or after the snapshot was taken.
  base::AutoLock lock(g_module_watcher_lock.Get());

  // Walk the module list snapshot.
  MODULEENTRY32 module = {sizeof(module)};
  for (BOOL result = ::Module32First(snapshot_handle.Get(), &module);
       result != FALSE;
       result = ::Module32Next(snapshot_handle.Get(), &module)) {
    ModuleEvent event(mojom::ModuleEventType::MODULE_ALREADY_LOADED,
                      base::FilePath(module.szExePath), module.modBaseAddr,
                      module.modBaseSize);

    // The snapshot must be reconciled with the pending module events to figure
    // out the current state of loaded modules.
    const auto& pending_event_iter = pending_events_->find(event);
    if (pending_event_iter != pending_events_->end()) {
      if (pending_event_iter->event_type ==
          mojom::ModuleEventType::MODULE_LOADED) {
        // If the module is in the snapshot, and it has a pending MODULE_LOADED
        // event, just ignore the pending event.
        callback_.Run(event);
      } else {
        // If the module is in the snapshot, and it has a pending
        // MODULE_UNLOADED event, send the MODULE_ALREADY_LOADED event followed
        // by the pending unload event.
        callback_.Run(event);
        callback_.Run(*pending_event_iter);
      }

      // The pending event was processed. Remove it from the list of pending
      // events so that unprocessed events can be identified.
      pending_events_->erase(pending_event_iter);
    } else {
      // There is no pending events associated to that module. Simply send the
      // MODULE_ALREADY_LOADED event.
      callback_.Run(event);
    }
  }

  // Process events that are still pending.
  for (const auto& pending_event : *pending_events_) {
    // A MODULE_ALREADY_LOADED event is sent in either case.
    ModuleEvent already_loaded_module_event(
        mojom::ModuleEventType::MODULE_ALREADY_LOADED,
        pending_event.module_path, pending_event.module_load_address,
        pending_event.module_size);

    if (pending_event.event_type == mojom::ModuleEventType::MODULE_LOADED) {
      // If the module is not in the snapshot, and it has a pending
      // MODULE_LOADED event, this means that the snapshot was taken before the
      // event happened. Send a MODULE_ALREADY_LOADED event instead of
      // MODULE_LOADED because the callback is not executed in the context of
      // the dll notification.
      callback_.Run(already_loaded_module_event);
    } else {
      // If the module is not in the snapshot, and it has a pending
      // MODULE_UNLOADED event, this means that the snapshot was taken after the
      // event happened, and the module was missed. Send a MODULE_ALREADY_LOADED
      // followed by the pending MODULE_UNLOADED event so that the callback is
      // aware of that module, event if it isn't loaded anymore.
      callback_.Run(already_loaded_module_event);
      callback_.Run(pending_event);
    }
  }

  pending_events_.reset();
  initialized_ = true;

  return;
}

// static
void __stdcall ModuleWatcher::LoaderNotificationCallback(
    unsigned long notification_reason,
    const LDR_DLL_NOTIFICATION_DATA* notification_data,
    void* context) {
  base::AutoLock lock(g_module_watcher_lock.Get());

  if (context != g_module_watcher_instance)
    return;

  // Convert the notification to a ModuleEvent.
  ModuleEvent module_event;
  switch (notification_reason) {
    case LDR_DLL_NOTIFICATION_REASON_LOADED:
      module_event = CreateModuleEvent(mojom::ModuleEventType::MODULE_LOADED,
                                       notification_data->Loaded);
      break;

    case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
      module_event = CreateModuleEvent(mojom::ModuleEventType::MODULE_UNLOADED,
                                       notification_data->Unloaded);
      break;

    default:
      // This is unexpected, but not a reason to crash.
      NOTREACHED() << "Unknown LDR_DLL_NOTIFICATION_REASON: "
                   << notification_reason;
      return;
  }

  // Depending on the state the of module watcher, either save the event for
  // later, or process it right now.
  ModuleWatcher* module_watcher = reinterpret_cast<ModuleWatcher*>(context);
  if (!module_watcher->initialized_) {
    // Always update the record.
    module_watcher->pending_events_->erase(module_event);
    module_watcher->pending_events_->insert(module_event);
  } else {
    module_watcher->callback_.Run(module_event);
  }
}

ModuleWatcher::ModuleWatcher(OnModuleEventCallback callback)
    : callback_(std::move(callback)),
      initialized_(false),
      pending_events_(
          base::MakeUnique<std::set<ModuleEvent, ModuleEventCompare>>()),
      dll_notification_cookie_(nullptr),
      register_dll_notification_callback_called_(false) {}
