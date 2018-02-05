// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define PATH_ELEMENT_TEMPLATE(path_name, ...) \
  static constexpr gfx::PathElement path_name[] = {__VA_ARGS__};

#define VECTOR_ICON_TEMPLATE(icon_name, path_name)                        \
  constexpr gfx::VectorIcon icon_name = {path_name, arraysize(path_name), \
                                         nullptr, 0u};

#define VECTOR_ICON_TEMPLATE2(icon_name, path_name, path_name_1x) \
  constexpr gfx::VectorIcon icon_name = {                         \
      path_name, arraysize(path_name), path_name_1x, arraysize(path_name_1x)};
