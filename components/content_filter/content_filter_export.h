// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_FILTER_CONTENT_FILTER_EXPORT_H_
#define COMPONENTS_CONTENT_FILTER_CONTENT_FILTER_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(CONTENT_FILTER_IMPLEMENTATION)
#define CONTENT_FILTER_EXPORT __declspec(dllexport)
#else
#define CONTENT_FILTER_EXPORT __declspec(dllimport)
#endif  // defined(CONTENT_FILTER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(CONTENT_FILTER_IMPLEMENTATION)
#define CONTENT_FILTER_EXPORT __attribute__((visibility("default")))
#else
#define CONTENT_FILTER_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define CONTENT_FILTER_EXPORT
#endif

#endif  // COMPONENTS_CONTENT_FILTER_CONTENT_FILTER_EXPORT_H_
