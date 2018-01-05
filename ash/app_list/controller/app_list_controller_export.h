// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_CONTROLLER_APP_LIST_CONTROLLER_EXPORT_H_
#define ASH_APP_LIST_CONTROLLER_APP_LIST_CONTROLLER_EXPORT_H_

// Defines APP_LIST_CONTROLLER_EXPORT so that functionality implemented by the
// app_list_controller module can be exported to consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(APP_LIST_CONTROLLER_IMPLEMENTATION)
#define APP_LIST_CONTROLLER_EXPORT __declspec(dllexport)
#else
#define APP_LIST_CONTROLLER_EXPORT __declspec(dllimport)
#endif  // defined(APP_LIST_CONTROLLER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(APP_LIST_CONTROLLER_IMPLEMENTATION)
#define APP_LIST_CONTROLLER_EXPORT __attribute__((visibility("default")))
#else
#define APP_LIST_CONTROLLER_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define APP_LIST_CONTROLLER_EXPORT
#endif

#endif  // ASH_APP_LIST_CONTROLLER_APP_LIST_CONTROLLER_EXPORT_H_
