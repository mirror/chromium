// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/automation_internal_custom_bindings.h"

namespace extensions {

AutomationAXTreeWrapper::AutomationAXTreeWrapper(
    int32_t tree_id,
    AutomationInternalCustomBindings* owner)
    : tree_id_(tree_id), parent_node_id_from_parent_tree_(-1), owner_(owner) {
  tree_.SetDelegate(owner);
}

}  // namespace extensions
