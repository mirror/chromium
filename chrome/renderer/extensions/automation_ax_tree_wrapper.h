// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_AUTOMATION_AX_TREE_WRAPPER_H_
#define CHROME_RENDERER_EXTENSIONS_AUTOMATION_AX_TREE_WRAPPER_H_

#include <map>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/common/extensions/api/automation.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "ipc/ipc_message.h"
#include "ui/accessibility/ax_tree.h"
#include "v8/include/v8.h"

namespace extensions {

class AutomationInternalCustomBindings;

// A class that wraps one AXTree and all of the additional state
// and helper methods needed to use it for the automation API.
class AutomationAXTreeWrapper {
 public:
  AutomationAXTreeWrapper(int tree_id, AutomationInternalCustomBindings* owner);
  ~AutomationAXTreeWrapper();

  int32_t tree_id() const { return tree_id_; }
  ui::AXTree* tree() { return &tree_; }
  AutomationInternalCustomBindings* owner() { return owner_; }

  // The host node ID is the node ID of the parent node in the parent tree.
  // For example, the host node ID of a web area of a child frame is the
  // ID of the <iframe> element in its parent frame.
  int32_t host_node_id() const { return host_node_id_; }
  void set_host_node_id(int32_t id) { host_node_id_ = id; }

 private:
  int32_t tree_id_;
  int32_t host_node_id_;
  ui::AXTree tree_;
  AutomationInternalCustomBindings* owner_;
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_AUTOMATION_AX_TREE_WRAPPER_H_
