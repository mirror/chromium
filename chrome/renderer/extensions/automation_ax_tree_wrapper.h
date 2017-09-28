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

class AutomationAXTreeWrapper {
 public:
  AutomationAXTreeWrapper(int tree_id, AutomationInternalCustomBindings* owner);
  ~AutomationAXTreeWrapper();

  int tree_id() const { return tree_id_; }
  ui::AXTree* tree() { return &tree_; }
  AutomationInternalCustomBindings* owner() { return owner_; }

  int parent_node_id_from_parent_tree() const {
    return parent_node_id_from_parent_tree_;
  }
  void set_parent_node_id_from_parent_tree(int id) {
    parent_node_id_from_parent_tree_ = id;
  }

 private:
  int tree_id_;
  int parent_node_id_from_parent_tree_;
  ui::AXTree tree_;
  AutomationInternalCustomBindings* owner_;
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_AUTOMATION_AX_TREE_WRAPPER_H_
