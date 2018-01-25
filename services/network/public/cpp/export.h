// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_EXPORT_H_
#define SERVICES_NETWORK_PUBLIC_CPP_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(SERVICES_NETWORK_PUBLIC_CPP_IMPL)
#define SERVICES_NETWORK_EXPORT __declspec(dllexport)
#else
#define SERVICES_NETWORK_EXPORT __declspec(dllimport)
#endif  // defined(SERVICES_NETWORK_PUBLIC_CPP_IMPL)

#else  // defined(WIN32)
#if defined(SERVICES_NETWORK_PUBLIC_CPP_IMPL)
#define SERVICES_NETWORK_EXPORT __attribute__((visibility("default")))
#else
#define SERVICES_NETWORK_EXPORT
#endif  // defined(SERVICES_NETWORK_PUBLIC_CPP_IMPL)
#endif

#else  // defined(COMPONENT_BUILD)
#define SERVICES_NETWORK_EXPORT
#endif

#endif  // SERVICES_NETWORK_PUBLIC_CPP_EXPORT_H_
