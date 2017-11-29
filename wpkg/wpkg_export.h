// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WPKG_WPKG_EXPORT_H_
#define WPKG_WPKG_EXPORT_H_

#if defined(COMPONENT_BUILD) && !defined(COMPILE_WPKG_STATICALLY)
#if defined(WIN32)

#if defined(WPKG_IMPLEMENTATION)
#define WPKG_EXPORT __declspec(dllexport)
#else
#define WPKG_EXPORT __declspec(dllimport)
#endif  // defined(WPKG_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(WPKG_IMPLEMENTATION)
#define WPKG_EXPORT __attribute__((visibility("default")))
#else
#define WPKG_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define WPKG_EXPORT
#endif

#endif  // WPKG_WPKG_EXPORT_H_
