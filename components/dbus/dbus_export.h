// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DBUS_DBUS_EXPORT_H_
#define COMPONENTS_DBUS_DBUS_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(COMPONENTS_DBUS_IMPLEMENTATION)
#define COMPONENTS_DBUS_EXPORT __attribute__((visibility("default")))
#else
#define COMPONENTS_DBUS_EXPORT
#endif  // defined(COMPONENTS_DBUS_IMPLEMENTATION)
#else   // defined(COMPONENT_BUILD)
#define COMPONENTS_DBUS_EXPORT
#endif

#endif  // COMPONENTS_DBUS_DBUS_EXPORT_H_
