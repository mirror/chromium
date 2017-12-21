// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_EXPORT_H_
#define COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_EXPORT_H_

// Defines KEYBOARD_SHORTCUT_VIEWER_EXPORT so that functionality implemented by
// the keyboard shortcut viewer module can be exported to consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(KEYBOARD_SHORTCUT_VIEWER_IMPLEMENTATION)
#define KEYBOARD_SHORTCUT_VIEWER_EXPORT __declspec(dllexport)
#else
#define KEYBOARD_SHORTCUT_VIEWER_EXPORT __declspec(dllimport)
#endif  // defined(KEYBOARD_SHORTCUT_VIEWER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(KEYBOARD_SHORTCUT_VIEWER_IMPLEMENTATION)
#define KEYBOARD_SHORTCUT_VIEWER_EXPORT __attribute__((visibility("default")))
#else
#define KEYBOARD_SHORTCUT_VIEWER_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define KEYBOARD_SHORTCUT_VIEWER_EXPORT
#endif

#endif  // COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_EXPORT_H_
