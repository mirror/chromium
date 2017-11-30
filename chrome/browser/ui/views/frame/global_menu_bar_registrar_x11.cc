// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/global_menu_bar_registrar_x11.h"

#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "base/logging.h"
#include "base/threading/thread.h"
#include "chrome/browser/ui/views/frame/global_menu_bar_x11.h"
#include "content/public/browser/browser_thread.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

using content::BrowserThread;

namespace {

const char kAppMenuRegistrarName[] = "com.canonical.AppMenu.Registrar";
const char kAppMenuRegistrarPath[] = "/com/canonical/AppMenu/Registrar";

void OnSignalConnectedDoNothing(const std::string&, const std::string&, bool) {}

void OnResponseDoNothing(dbus::Response*) {}

}  // namespace

// static
GlobalMenuBarRegistrarX11* GlobalMenuBarRegistrarX11::GetInstance() {
  return base::Singleton<GlobalMenuBarRegistrarX11>::get();
}

void GlobalMenuBarRegistrarX11::OnWindowMapped(unsigned long xid) {
  live_xids_.insert(xid);

  if (registrar_proxy_)
    RegisterXID(xid);
}

void GlobalMenuBarRegistrarX11::OnWindowUnmapped(unsigned long xid) {
  if (registrar_proxy_)
    UnregisterXID(xid);

  live_xids_.erase(xid);
}

GlobalMenuBarRegistrarX11::GlobalMenuBarRegistrarX11()
    : registrar_proxy_(nullptr) {
  base::Thread::Options thread_options;
  thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  dbus_thread_.reset(new base::Thread("D-Bus menu bar registrar thread"));
  dbus_thread_->StartWithOptions(thread_options);

  dbus::Bus::Options bus_options;
  bus_options.bus_type = dbus::Bus::SESSION;
  bus_options.connection_type = dbus::Bus::SHARED;
  bus_options.dbus_task_runner = dbus_thread_->task_runner();
  bus_ = base::MakeRefCounted<dbus::Bus>(bus_options);

  registrar_proxy_ = bus_->GetObjectProxy(
      kAppMenuRegistrarName, dbus::ObjectPath(kAppMenuRegistrarPath));
  // GetObjectProxy() should never return null.
  DCHECK(registrar_proxy_);

  registrar_proxy_->ConnectToSignal(
      kAppMenuRegistrarName, "g-name-owner",
      base::Bind(&GlobalMenuBarRegistrarX11::OnNameOwnerChanged,
                 base::Unretained(this)),
      base::Bind(OnSignalConnectedDoNothing));
  OnNameOwnerChanged(nullptr);
}

GlobalMenuBarRegistrarX11::~GlobalMenuBarRegistrarX11() {
  bus_->ShutdownOnDBusThreadAndBlock();
  dbus_thread_->Stop();
}

void GlobalMenuBarRegistrarX11::RegisterXID(unsigned long xid) {
  DCHECK(registrar_proxy_);
  std::string path = GlobalMenuBarX11::GetPathForWindow(xid);

  dbus::MethodCall register_window_call(kAppMenuRegistrarName,
                                        "RegisterWindow");
  dbus::MessageWriter writer(&register_window_call);
  writer.AppendUint32(xid);
  writer.AppendObjectPath(dbus::ObjectPath(path));

  registrar_proxy_->CallMethod(&register_window_call,
                               dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                               base::BindOnce(&OnResponseDoNothing));
}

void GlobalMenuBarRegistrarX11::UnregisterXID(unsigned long xid) {
  DCHECK(registrar_proxy_);
  std::string path = GlobalMenuBarX11::GetPathForWindow(xid);

  dbus::MethodCall unregister_window_call(kAppMenuRegistrarName,
                                          "UnregisterWindow");
  dbus::MessageWriter writer(&unregister_window_call);
  writer.AppendUint32(xid);

  registrar_proxy_->CallMethod(&unregister_window_call,
                               dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                               base::BindOnce(&OnResponseDoNothing));
}

void GlobalMenuBarRegistrarX11::OnNameOwnerChanged(dbus::Signal*) {
  // If the name owner changed, we need to reregister all the live xids with
  // the system.
  for (std::set<unsigned long>::const_iterator it = live_xids_.begin();
       it != live_xids_.end(); ++it) {
    RegisterXID(*it);
  }
}
