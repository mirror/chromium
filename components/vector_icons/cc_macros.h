// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file holds macros that are common to each vector icon target's
// vector_icons.cc.template file.

// If the vector_icons.cc.template file doesn't define a prefix, we'll go
// without one.
#ifndef VECTOR_ICON_ID_PREFIX
#define VECTOR_ICON_ID_PREFIX ""
#endif

#define PATH_ELEMENT_TEMPLATE(path_name, ...) \
  static constexpr gfx::PathElement path_name[] = {__VA_ARGS__};

// The VectorIcon will be called kMyIcon, and the identifier for the icon might
// be "my_namespace::kMyIconId".
#define VECTOR_ICON_TEMPLATE(icon_name, path_name)                        \
  const char icon_name##Id[] = VECTOR_ICON_ID_PREFIX #icon_name;          \
  constexpr gfx::VectorIcon icon_name = {path_name, arraysize(path_name), \
                                         nullptr, 0u, icon_name##Id};

#define VECTOR_ICON_TEMPLATE2(icon_name, path_name, path_name_1x)             \
  const char icon_name##Id[] = VECTOR_ICON_ID_PREFIX #icon_name;              \
  constexpr gfx::VectorIcon icon_name = {                                     \
      path_name, arraysize(path_name), path_name_1x, arraysize(path_name_1x), \
      icon_name##Id};
