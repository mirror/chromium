// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DBUS_THREAD_DBUS_THREAD_H_
#define COMPONENTS_DBUS_THREAD_DBUS_THREAD_H_

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "components/dbus_thread/dbus_thread_export.h"

namespace dbus_thread {

DBUS_THREAD_EXPORT scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner();

}  // namespace dbus_thread

#endif  // COMPONENTS_DBUS_THREAD_DBUS_THREAD_H_
