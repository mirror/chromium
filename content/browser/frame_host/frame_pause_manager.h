// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_PAUSE_MANAGER_H_
#define CONTENT_BROWSER_FRAME_PAUSE_MANAGER_H_

#include <queue>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

namespace content {

class FrameTree;
class FrameTreeNode;

// TODO(jkarlin): Go back to using FrameHost and Frame mojo interfaces..
// See if we can send a message per frame proxy instead of directly to each
// frame. Let the render process distribute the message amongst its frames.
// That's the pattern that dcheng expects to see.

// FramePauseManager is a per-FrameTree object that runs frame pause and
// unpause operations in sequential order. This is necessary because the
// operations are async and if multiple operations run in parallel madness may
// ensue if they touch the same parts of the tree.
class FramePauseManager {
 public:
  explicit FramePauseManager(FrameTree* frame_tree);
  ~FramePauseManager();

  void PauseFrame(base::WeakPtr<FrameTreeNode> frame_tree_node,
                  base::OnceClosure callback);
  void UnpauseFrame(base::WeakPtr<FrameTreeNode> frame_tree_node,
                    base::OnceClosure callback);

 private:
  void RunNextOperation();

  void PauseOperationImpl(FrameTreeNode* frame_tree_node,
                          base::OnceClosure callback);
  void UnpauseOperationImpl(FrameTreeNode* frame_tree_node,
                            base::OnceClosure callback);
  void OperationComplete(base::OnceClosure callback);

  struct Operation {
    enum class OperationType { PAUSE = 0, UNPAUSE };
    Operation(OperationType operation_type,
              base::WeakPtr<FrameTreeNode> frame_tree_node,
              base::OnceClosure callback);
    ~Operation();
    OperationType operation_type;
    base::WeakPtr<FrameTreeNode> frame_tree_node;
    base::OnceClosure callback;
  };

  // |this| is owned by |frame_tree_|.
  FrameTree* frame_tree_;
  std::queue<Operation> pending_operations_;
  bool operation_in_progress_ = false;
  base::WeakPtrFactory<FramePauseManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FramePauseManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_PAUSE_MANAGER_H_
