// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_EXPORT_H_
#define SERVICES_NETWORK_PUBLIC_CPP_NETWORK_EXPORT_H_

#if defined(COMPONENT_BUILD)

#if defined(WIN32)

#if defined(NETWORK_IMPLEMENTATION)
#define NETWORK_EXPORT __declspec(dllexport)
#else
#define NETWORK_EXPORT __declspec(dllimport)
#endif

#else  // !defined(WIN32)

#if defined(NETWORK_IMPLEMENTATION)
#define NETWORK_EXPORT __attribute__((visibility("default")))
#else
#define NETWORK_EXPORT
#endif

#endif

#else  // !defined(COMPONENT_BUILD)

#define NETWORK_EXPORT

#endif

#endif  // SERVICES_NETWORK_PUBLIC_CPP_NETWORK_EXPORT_H_
