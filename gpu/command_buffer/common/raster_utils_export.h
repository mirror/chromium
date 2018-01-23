// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_RASTER_UTILS_EXPORT_H_
#define GPU_COMMAND_BUFFER_COMMON_RASTER_UTILS_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(RASTER_UTILS_IMPLEMENTATION)
#define RASTER_UTILS_EXPORT __declspec(dllexport)
#else
#define RASTER_UTILS_EXPORT __declspec(dllimport)
#endif  // defined(RASTER_UTILS_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(RASTER_UTILS_IMPLEMENTATION)
#define RASTER_UTILS_EXPORT __attribute__((visibility("default")))
#else
#define RASTER_UTILS_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define RASTER_UTILS_EXPORT
#endif

#endif  // GPU_COMMAND_BUFFER_COMMON_RASTER_UTILS_EXPORT_H_
