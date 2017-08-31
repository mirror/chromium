// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_EXPORT_H_
#define COMPONENTS_ARC_ARC_EXPORT_H_

#if defined(COMPONENT_BUILD) && defined(ARC_IMPLEMENTATION)

#if !defined(OS_CHROMEOS)
#error "ARC can be built only for ChromeOS."
#endif  // defined(OS_CHROMEOS)
#define ARC_EXPORT __attribute__((visibility("default")))

#else  // defined(COMPONENT_BUILD) && defined(ARC_IMPLEMENTATION)

// Note that this can be included in any platform build, just for convenience.
#define ARC_EXPORT

#endif

#endif  // COMPONENTS_ARC_ARC_EXPORT_H_
