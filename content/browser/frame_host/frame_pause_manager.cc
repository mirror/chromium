// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/frame_pause_manager.h"

#include "base/barrier_closure.h"
#include "base/callback_helpers.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_impl.h"

namespace content {

FramePauseManager::FramePauseManager(FrameTree* frame_tree)
    : frame_tree_(frame_tree), weak_ptr_factory_(this) {}
FramePauseManager::~FramePauseManager() {
  // TODO(jkarlin): Run all of the callbacks!
}

void FramePauseManager::PauseFrame(base::WeakPtr<FrameTreeNode> frame_tree_node,
                                   bool rendering,
                                   bool loading,
                                   bool script,
                                   base::OnceClosure callback) {
  pending_operations_.emplace(Operation::OperationType::PAUSE, frame_tree_node,
                              rendering, loading, script, std::move(callback));
  if (!operation_in_progress_)
    RunNextOperation();
}

void FramePauseManager::UnpauseFrame(
    base::WeakPtr<FrameTreeNode> frame_tree_node,
    base::OnceClosure callback) {
  pending_operations_.emplace(Operation::OperationType::UNPAUSE,
                              frame_tree_node, false, false, false,
                              std::move(callback));
  if (!operation_in_progress_)
    RunNextOperation();
}

void FramePauseManager::RunNextOperation() {
  DCHECK(!operation_in_progress_);

  // Find an operation to run and run it.
  while (!pending_operations_.empty()) {
    Operation& next_op = pending_operations_.front();

    if (!next_op.frame_tree_node) {
      // The frame has since dissappeared.
      std::move(next_op.callback).Run();
      pending_operations_.pop();
      continue;
    }
    // Found an operation to run.
    switch (next_op.operation_type) {
      case Operation::OperationType::PAUSE:
        PauseOperationImpl(next_op.frame_tree_node.get(),
                           next_op.pause_rendering, next_op.pause_loading,
                           next_op.pause_script, std::move(next_op.callback));
        break;
      case Operation::OperationType::UNPAUSE:
        UnpauseOperationImpl(next_op.frame_tree_node.get(),
                             std::move(next_op.callback));
        break;
    }
    pending_operations_.pop();

    // Break out of the loop so that we only run one operation at a time.
    return;
  }
}

void FramePauseManager::PauseOperationImpl(FrameTreeNode* frame_tree_node,
                                           bool rendering,
                                           bool loading,
                                           bool script,
                                           base::OnceClosure callback) {
  operation_in_progress_ = true;
  LOG(ERROR) << "Pausing";
  // Walk the tree and increment each one.
  // hmm, by the time we've run this, the RFH could be completely different
  // from the one that we originally tried to pause.
  // e.g., 1. pause the frame, 2., renavigate the frame, 3. the pause happens
  // on the renavigated page. That seems a bit strange.

  // Increment the pause count for each node and keep track of which frames
  // will have to be paused.
  std::vector<FrameTreeNode*> async_nodes;
  for (FrameTreeNode* node : frame_tree_->SubtreeNodes(frame_tree_node)) {
    async_nodes.push_back(node);
  }

  // base::Unretained is okay since the closures will all be called by the RDHs
  // when they're closed, which happens before FramePauseManager is
  // destroyed. TODO(jkarlin): verify this.
  base::RepeatingClosure barrier_closure = BarrierClosure(
      async_nodes.size(),
      base::BindOnce(&FramePauseManager::OperationComplete,
                     base::Unretained(this), std::move(callback)));

  for (FrameTreeNode* node : async_nodes) {
    RenderFrameHostImpl* frame_host = node->current_frame_host();
    // We need to guarantee that the barrier closure is run even if the mojo
    // call fails. So we stuff it into a ScopedClosureRunner, which requires a
    // OnceClosure, so we first stuff the barrier closure into a OnceClosure.
    // TODO(jkarlin): Find a cleaner way.
    base::ScopedClosureRunner scoped_barrier_callback(base::BindOnce(
        [](const base::RepeatingClosure& closure) { closure.Run(); },
        barrier_closure));

    // The callback must be run (even if the renderer dissappears) else this
    // operation will never complete. Guarantee that it's eventually run even
    // if a mojo connection fails by wrapping the closure in a
    // ScopedClosureRunner.
    frame_host->PauseFrame(rendering, loading, script,
                           std::move(scoped_barrier_callback));
  }
}

void FramePauseManager::UnpauseOperationImpl(FrameTreeNode* frame_tree_node,
                                             base::OnceClosure callback) {
  operation_in_progress_ = true;
  // Walk the tree and increment each one.
  // hmm, by the time we've run this, the RFH could be completely different
  // from the one that we originally tried to pause.
  // e.g., 1. pause the frame, 2., renavigate the frame, 3. the pause happens
  // on the renavigated page. That seems a bit strange.

  // Increment the pause count for each node and keep track of which frames
  // will have to be paused.
  std::vector<FrameTreeNode*> async_nodes;
  for (FrameTreeNode* node : frame_tree_->SubtreeNodes(frame_tree_node))
    async_nodes.push_back(node);

  // base::Unretained is okay since the closures will all be called by the RDHs
  // when they're closed, which happens before FramePauseManager is
  // destroyed. TODO(jkarlin): verify this.
  base::RepeatingClosure barrier_closure = BarrierClosure(
      async_nodes.size(),
      base::BindOnce(&FramePauseManager::OperationComplete,
                     base::Unretained(this), std::move(callback)));

  for (FrameTreeNode* node : async_nodes) {
    RenderFrameHostImpl* frame_host = node->current_frame_host();
    // We need to guarantee that the barrier closure is run even if the mojo
    // call fails. So we stuff it into a ScopedClosureRunner, which requires a
    // OnceClosure, so we first stuff the barrier closure into a OnceClosure.
    // TODO(jkarlin): Find a cleaner way.
    base::ScopedClosureRunner scoped_barrier_callback(base::BindOnce(
        [](const base::RepeatingClosure& closure) { closure.Run(); },
        barrier_closure));

    // The callback must be run (even if the renderer dissappears) else this
    // operation will never complete. Guarantee that it's eventually run even
    // if a mojo connection fails by wrapping the closure in a
    // ScopedClosureRunner.
    frame_host->UnpauseFrame(std::move(scoped_barrier_callback));
  }
}

void FramePauseManager::OperationComplete(base::OnceClosure callback) {
  std::move(callback).Run();
  operation_in_progress_ = false;
  // Posttask to avoid reentrancy.
  base::PostTask(FROM_HERE, base::BindOnce(&FramePauseManager::RunNextOperation,
                                           weak_ptr_factory_.GetWeakPtr()));
}

FramePauseManager::Operation::Operation(
    OperationType operation_type,
    base::WeakPtr<FrameTreeNode> frame_tree_node,
    bool rendering,
    bool loading,
    bool script,
    base::OnceClosure callback)
    : operation_type(operation_type),
      frame_tree_node(frame_tree_node),
      pause_rendering(rendering),
      pause_loading(loading),
      pause_script(script),
      callback(std::move(callback)) {}

FramePauseManager::Operation::~Operation() = default;

}  // namespace content
