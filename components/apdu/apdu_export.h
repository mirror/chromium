// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_APDU_APDU_EXPORT_H_
#define COMPONENTS_APDU_APDU_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(APDU_IMPLEMENTATION)
#define APDU_EXPORT __declspec(dllexport)
#else
#define APDU_EXPORT __declspec(dllimport)
#endif  // defined(CBOR_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(APDU_IMPLEMENTATION)
#define APDU_EXPORT __attribute__((visibility("default")))
#else
#define APDU_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define APDU_EXPORT
#endif

#endif  // COMPONENTS_APDU_APDU_EXPORT_H_
