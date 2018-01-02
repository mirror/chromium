// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef URL_DEBUG_ALIAS_HELPER_H_
#define URL_DEBUG_ALIAS_HELPER_H_

#include "base/debug/alias.h"
#include "base/strings/string_util.h"

// Helper macro used by DEBUG_ALIAS_FOR_ORIGIN from url/origin.h.
// TODO(lukasza): Move this to base/debug/alias.h
#define DEBUG_ALIAS_FOR_CSTR(var_name, c_str, char_count) \
  char var_name[char_count];                              \
  base::strlcpy(var_name, (c_str), arraysize(var_name));  \
  base::debug::Alias(var_name);

#endif  // URL_DEBUG_ALIAS_HELPER_H_
