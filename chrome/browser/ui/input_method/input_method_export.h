// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_INPUT_METHOD_INPUT_METHOD_EXPORT_H_
#define CHROME_BROWSER_UI_INPUT_METHOD_INPUT_METHOD_EXPORT_H_

// Defines INPUT_METHOD_EXPORT so that the classes in the input_method module
// can be exposed to consumer.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(INPUT_METHOD_IMPLEMENTATION)
#define INPUT_METHOD_EXPORT __declspec(dllexport)
#else
#define INPUT_METHOD_EXPORT __declspec(dllimport)
#endif  // defined(INPUT_METHOD_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(INPUT_METHOD_IMPLEMENTATION)
#define INPUT_METHOD_EXPORT __attribute__((visibility("default")))
#else
#define INPUT_METHOD_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define INPUT_METHOD_EXPORT
#endif

#endif  // CHROME_BROWSER_UI_INPUT_METHOD_INPUT_METHOD_EXPORT_H_
