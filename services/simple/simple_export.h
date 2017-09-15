// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SIMPLE_SIMPLE_EXPORT_H
#define SERVICES_SIMPLE_SIMPLE_EXPORT_H

// Use this when implementing the createXXX() function to ensure their
// symbols are properly exported by their shared library.
#if defined(WIN32)
#if defined(SIMPLE_API_IMPLEMENTATION)
#define SIMPLE_EXPORT __declspec(dllexport)
#else
#define SIMPLE_EXPORT __declspec(dllimport)
#endif
#else
#if defined(SIMPLE_API_IMPLEMENTATION)
#define SIMPLE_EXPORT __attribute__((visibility("default")))
#else
#define SIMPLE_EXPORT /* nothing */
#endif
#endif

#endif  // SERVICES_SIMPLE_SIMPLE_EXPORT_H
