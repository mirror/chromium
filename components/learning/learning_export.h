// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LEARNING_LEARNING_EXPORT_H_
#define COMPONENTS_LEARNING_LEARNING_EXPORT_H_

#if defined(COMPONENT_BUILD)

#if defined(WIN32)

#if defined(LEARNING_COMPONENT_IMPLEMENTATION)
#define LEARNING_EXPORT __declspec(dllexport)
#else
#define LEARNING_EXPORT __declspec(dllimport)
#endif  // defined(LEARNING_COMPONENT_IMPLEMENTATION)

#else  // defined(WIN32)

#if defined(LEARNING_COMPONENT_IMPLEMENTATION)
#define LEARNING_EXPORT __attribute__((visibility("default")))
#else
#define LEARNING_EXPORT
#endif  // defined(LEARNING_COMPONENT_IMPLEMENTATION)

#endif  // defined(WIN32)

#else  // defined(COMPONENT_BUILD)

#define LEARNING_EXPORT

#endif  // defined(COMPONENT_BUILD)

#endif  // COMPONENTS_LEARNING_LEARNING_EXPORT_H_
