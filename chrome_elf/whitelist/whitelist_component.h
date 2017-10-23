// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_COMPONENT_H_
#define CHROME_ELF_WHITELIST_WHITELIST_COMPONENT_H_

namespace whitelist {

enum ComponentStatus {
  kComponentSuccess = 0,
};

// Initialize component whitelist.
ComponentStatus InitComponent();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_COMPONENT_H_
