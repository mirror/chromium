// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DBUS_DBUS_THREAD_LINUX_H_
#define COMPONENTS_DBUS_DBUS_THREAD_LINUX_H_

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "components/dbus/dbus_export.h"

// Many APIs in ::dbus are required to be called from the same thread
// (https://crbug.com/130984). Therefore, a SingleThreadedTaskRunner is
// maintained and accessible through GetDBusTaskRunner(), from which all calls
// to dbus on Linux have to be made.

#if defined(OS_CHROMEOS)
#error On ChromeOS, use DBusThreadManager instead.
#endif

namespace dbus_thread_linux {

COMPONENTS_DBUS_EXPORT scoped_refptr<base::SingleThreadTaskRunner>
GetDBusTaskRunner();

}  // namespace dbus_thread_linux

#endif  // COMPONENTS_DBUS_DBUS_THREAD_LINUX_H_
