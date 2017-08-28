// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_WEBAGENTS_EXPORT_H_
#define WEBAGENTS_WEBAGENTS_EXPORT_H_

namespace webagents {

// This macro is intended to export symbols in Source/web/ which are still
// private to Blink (for instance, because they are used in unit tests).

#if defined(COMPONENT_BUILD)
#if defined(WIN32)
#if WEBAGENTS_IMPLEMENTATION
#define WEBAGENTS_EXPORT __declspec(dllexport)
#else
#define WEBAGENTS_EXPORT __declspec(dllimport)
#endif  // BLINK_WEB_IMPLEMENTATION
#else   // defined(WIN32)
#define WEBAGENTS_EXPORT __attribute__((visibility("default")))
#endif
#else  // defined(COMPONENT_BUILD)
#define WEBAGENTS_EXPORT
#endif

}  // namespace webagents

#endif  // WEBAGENTS_WEBAGENTS_EXPORT_H_
