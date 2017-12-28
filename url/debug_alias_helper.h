// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef URL_DEBUG_ALIAS_HELPER_H_
#define URL_DEBUG_ALIAS_HELPER_H_

#include <string.h>

#include "base/debug/alias.h"

// Helper macro used by DEBUG_ALIAS_FOR_GURL and DEBUG_ALIAS_FOR_ORIGIN
// TODO(lukasza): Move this to base/debug/alias.h
#define DEBUG_ALIAS_FOR_CSTR(var_name, c_str, char_count) \
  char var_name[char_count];                              \
  strncpy(var_name, c_str, char_count - 1);               \
  var_name[char_count - 1] = '\0';                        \
  base::debug::Alias(var_name);

#endif  // URL_DEBUG_ALIAS_HELPER_H_
