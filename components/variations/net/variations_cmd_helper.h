// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_NET_VARIATIONS_CMD_HELPER_H_
#define COMPONENTS_VARIATIONS_NET_VARIATIONS_CMD_HELPER_H_

#include <string>

namespace variations {

// Generate a string containing the complete state of variations (including all
// the registered trials with corresponding groups, params and features) for the
// client in the command-line format.
void GetVariationsCmd(std::string* output);

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_NET_VARIATIONS_CMD_HELPER_H_