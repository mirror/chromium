// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_URL_LOG_EXPORT_H_
#define COMPONENTS_URL_LOG_EXPORT_H_

#if defined(COMPONENT_BUILD)

#if defined(WIN32)

#if defined(URL_LOG_IMPLEMENTATION)
#define URL_LOG_EXPORT __declspec(dllexport)
#else
#define URL_LOG_EXPORT __declspec(dllimport)
#endif  // defined(URL_LOG_IMPLEMENTATION)

#else  // defined(WIN32)

#if defined(URL_LOG_IMPLEMENTATION)
#define URL_LOG_EXPORT __attribute__((visibility("default")))
#else
#define URL_LOG_EXPORT
#endif

#endif

#else  // defined(COMPONENT_BUILD)

#define URL_LOG_EXPORT

#endif

#endif  // COMPONENTS_URL_LOG_EXPORT_H_
