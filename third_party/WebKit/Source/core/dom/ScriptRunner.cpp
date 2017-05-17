/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/dom/ScriptRunner.h"

#include <algorithm>
#include "bindings/core/v8/ScriptStreamer.h"
#include "bindings/core/v8/V8BindingForCore.h"  // for ToScriptStateForMainWorld
#include "core/dom/Document.h"
#include "core/dom/ScriptLoader.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/frame/LocalFrame.h"  // for GetFrame()->GetSettings()
#include "platform/heap/Handle.h"
#include "platform/scheduler/child/web_scheduler.h"
#include "public/platform/Platform.h"
#include "public/platform/WebThread.h"

namespace blink {

ScriptRunner::ScriptRunner(Document* document)
    : document_(document),
      task_runner_(TaskRunnerHelper::Get(TaskType::kNetworking, document)),
      currently_streamed_script_was_notify_finished_(false),
      number_of_in_order_scripts_with_pending_notification_(0),
      is_suspended_(false) {
  DCHECK(document);
#ifndef NDEBUG
  number_of_extra_tasks_ = 0;
#endif
}

void ScriptRunner::QueueScriptForExecution(ScriptLoader* script_loader,
                                           AsyncExecutionType execution_type) {
  DCHECK(script_loader);
  document_->IncrementLoadEventDelayCount();
  switch (execution_type) {
    case kAsync:
      pending_async_scripts_.insert(script_loader);
      TryStream(script_loader, true);
      break;

    case kInOrder:
      pending_in_order_scripts_.push_back(script_loader);
      number_of_in_order_scripts_with_pending_notification_++;
      break;
    case kNone:
      NOTREACHED();
      break;
  }
}

void ScriptRunner::PostTask(const WebTraceLocation& web_trace_location) {
  task_runner_->PostTask(
      web_trace_location,
      WTF::Bind(&ScriptRunner::ExecuteTask, WrapWeakPersistent(this)));
}

void ScriptRunner::Suspend() {
#ifndef NDEBUG
  // Resume will re-post tasks for all available scripts.
  number_of_extra_tasks_ += async_scripts_to_execute_soon_.size() +
                            in_order_scripts_to_execute_soon_.size() +
                            (currently_streamed_script_ &&
                             currently_streamed_script_was_notify_finished_);
#endif

  is_suspended_ = true;
}

void ScriptRunner::Resume() {
  DCHECK(is_suspended_);

  is_suspended_ = false;

  for (size_t i = 0; i < async_scripts_to_execute_soon_.size(); ++i) {
    PostTask(BLINK_FROM_HERE);
  }
  for (size_t i = 0; i < in_order_scripts_to_execute_soon_.size(); ++i) {
    PostTask(BLINK_FROM_HERE);
  }
}

void ScriptRunner::ScheduleReadyInOrderScripts() {
  while (!pending_in_order_scripts_.IsEmpty() &&
         pending_in_order_scripts_.front()->IsReady()) {
    // A ScriptLoader that failed is responsible for cancelling itself
    // notifyScriptLoadError(); it continues this draining of ready scripts.
    if (pending_in_order_scripts_.front()->ErrorOccurred())
      break;
    in_order_scripts_to_execute_soon_.push_back(
        pending_in_order_scripts_.TakeFirst());
    PostTask(BLINK_FROM_HERE);
  }
}

void ScriptRunner::NotifyScriptReady(ScriptLoader* script_loader,
                                     AsyncExecutionType execution_type) {
  if (script_loader == currently_streamed_script_) {
    DCHECK(!currently_streamed_script_was_notify_finished_);
    currently_streamed_script_was_notify_finished_ = true;
    return;
  }

  SECURITY_CHECK(script_loader);
  switch (execution_type) {
    case kAsync:
      // SECURITY_CHECK() makes us crash in a controlled way in error cases
      // where the ScriptLoader is associated with the wrong ScriptRunner
      // (otherwise we'd cause a use-after-free in ~ScriptRunner when it tries
      // to detach).
      SECURITY_CHECK(pending_async_scripts_.Contains(script_loader));

      pending_async_scripts_.erase(script_loader);
      async_scripts_to_execute_soon_.push_back(script_loader);

      PostTask(BLINK_FROM_HERE);
      TryStreamMore();
      break;

    case kInOrder:
      SECURITY_CHECK(number_of_in_order_scripts_with_pending_notification_ > 0);
      number_of_in_order_scripts_with_pending_notification_--;

      ScheduleReadyInOrderScripts();

      break;
    case kNone:
      NOTREACHED();
      break;
  }
}

bool ScriptRunner::RemovePendingInOrderScript(ScriptLoader* script_loader) {
  auto it = std::find(pending_in_order_scripts_.begin(),
                      pending_in_order_scripts_.end(), script_loader);
  if (it == pending_in_order_scripts_.end())
    return false;
  pending_in_order_scripts_.erase(it);
  SECURITY_CHECK(number_of_in_order_scripts_with_pending_notification_ > 0);
  number_of_in_order_scripts_with_pending_notification_--;
  return true;
}

void ScriptRunner::NotifyScriptLoadError(ScriptLoader* script_loader,
                                         AsyncExecutionType execution_type) {
  if (script_loader == currently_streamed_script_) {
    DCHECK_NE(streamers_.find(currently_streamed_script_), streamers_.end());
    streamers_.find(currently_streamed_script_)->value->Cancel();
    currently_streamed_script_ = nullptr;
    currently_streamed_script_was_notify_finished_ = false;
    document_->DecrementLoadEventDelayCount();
    TryStreamMore();
    return;
  }

  switch (execution_type) {
    case kAsync: {
      // See notifyScriptReady() comment.
      SECURITY_CHECK(pending_async_scripts_.Contains(script_loader));
      pending_async_scripts_.erase(script_loader);
      break;
    }
    case kInOrder: {
      SECURITY_CHECK(RemovePendingInOrderScript(script_loader));
      ScheduleReadyInOrderScripts();
      break;
    }
    case kNone: {
      NOTREACHED();
      break;
    }
  }
  document_->DecrementLoadEventDelayCount();
}

void ScriptRunner::NotifyScriptStreamerFinished() {
  if (!currently_streamed_script_) {
    return;
  }

  DCHECK(currently_streamed_script_);
  ScriptLoader* script_loader = currently_streamed_script_;
  currently_streamed_script_ = nullptr;

  if (currently_streamed_script_was_notify_finished_) {
    async_scripts_to_execute_soon_.push_back(script_loader);
    if (!is_suspended_)
      PostTask(BLINK_FROM_HERE);
  } else {
    pending_async_scripts_.insert(script_loader);
  }

  TryStreamMore();
}

void ScriptRunner::MovePendingScript(Document& old_document,
                                     Document& new_document,
                                     ScriptLoader* script_loader) {
  Document* new_context_document = new_document.ContextDocument();
  if (!new_context_document) {
    // Document's contextDocument() method will return no Document if the
    // following conditions both hold:
    //
    //   - The Document wasn't created with an explicit context document
    //     and that document is otherwise kept alive.
    //   - The Document itself is detached from its frame.
    //
    // The script element's loader is in that case moved to document() and
    // its script runner, which is the non-null Document that contextDocument()
    // would return if not detached.
    DCHECK(!new_document.GetFrame());
    new_context_document = &new_document;
  }
  Document* old_context_document = old_document.ContextDocument();
  if (!old_context_document) {
    DCHECK(!old_document.GetFrame());
    old_context_document = &old_document;
  }
  if (old_context_document != new_context_document)
    old_context_document->GetScriptRunner()->MovePendingScript(
        new_context_document->GetScriptRunner(), script_loader);
}

void ScriptRunner::MovePendingScript(ScriptRunner* new_runner,
                                     ScriptLoader* script_loader) {
  auto it = pending_async_scripts_.find(script_loader);
  if (it != pending_async_scripts_.end()) {
    new_runner->QueueScriptForExecution(script_loader, kAsync);
    pending_async_scripts_.erase(it);
    document_->DecrementLoadEventDelayCount();
    return;
  }
  if (RemovePendingInOrderScript(script_loader)) {
    new_runner->QueueScriptForExecution(script_loader, kInOrder);
    document_->DecrementLoadEventDelayCount();
  }
}

// Returns true if task was run, and false otherwise.
bool ScriptRunner::ExecuteTaskFromQueue(
    HeapDeque<Member<ScriptLoader>>* task_queue) {
  if (task_queue->IsEmpty())
    return false;

  ScriptLoader* task = task_queue->TakeFirst();

  ScriptStreamer* streamer = nullptr;
  auto streamer_it = streamers_.find(task);
  if (streamer_it != streamers_.end()) {
    streamer = streamer_it->value;
    streamers_.erase(streamer_it);
  }
  task->ExecuteWithScriptStreamer(streamer);

  document_->DecrementLoadEventDelayCount();
  return true;
}

void ScriptRunner::ExecuteTask() {
  if (is_suspended_)
    return;

  if (ExecuteTaskFromQueue(&async_scripts_to_execute_soon_))
    return;

  if (ExecuteTaskFromQueue(&in_order_scripts_to_execute_soon_))
    return;

#ifndef NDEBUG
  // Extra tasks should be posted only when we resume after suspending.
  // Or should be accounted for in number_of_extra_tasks_;
  DCHECK_GT(number_of_extra_tasks_--, 0);
#endif
}

void ScriptRunner::TryStreamMore() {
  if (is_suspended_)
    return;

  // Look through async_scripts_to_execute_soon_, and stream any one of them.
  for (auto it = async_scripts_to_execute_soon_.begin();
       !currently_streamed_script_ &&
       it != async_scripts_to_execute_soon_.end();
       ++it) {
    TryStream(*it, false);
  }
}

void ScriptRunner::TryStream(ScriptLoader* script_loader, bool is_pending) {
  DCHECK(script_loader);

  if (is_suspended_)
    return;

  // If we're already streaming, we can't stream another one.
  if (currently_streamed_script_)
    return;

  // We shouldn't re-parse an already parsed file.
  // if (script_loader->HasStreamer())
  //  return;
  if (streamers_.find(script_loader) != streamers_.end())
    return;

  // No resource -> can't stream.
  if (!script_loader->GetResource())
    return;

  // Try to obtain ScriptState (needed for the streamer);
  if (!document_->GetFrame())
    return;
  ScriptState* script_state = ToScriptStateForMainWorld(document_->GetFrame());
  if (!script_state)
    return;

  ScriptStreamer* streamer = ScriptStreamer::StartStreaming(
      script_loader->GetResource(), ScriptStreamer::kAsync,
      WTF::Bind(&ScriptRunner::NotifyScriptStreamerFinished,
                WrapPersistent(this)),
      document_->GetFrame()->GetSettings(), script_state,
      TaskRunnerHelper::Get(TaskType::kNetworkingControl, document_));

  if (streamer) {
    currently_streamed_script_ = script_loader;
    currently_streamed_script_was_notify_finished_ = !is_pending;

    streamers_.insert(script_loader, streamer);

    if (!is_pending) {
      auto it = std::find(async_scripts_to_execute_soon_.begin(),
                          async_scripts_to_execute_soon_.end(), script_loader);
      DCHECK(it != async_scripts_to_execute_soon_.end());
      async_scripts_to_execute_soon_.erase(it);
#ifndef NDEBUG
      number_of_extra_tasks_++;
#endif
    } else {
      DCHECK_NE(pending_async_scripts_.find(script_loader),
                pending_async_scripts_.end());
      pending_async_scripts_.erase(script_loader);
    }
  }
}

DEFINE_TRACE(ScriptRunner) {
  visitor->Trace(document_);
  visitor->Trace(pending_in_order_scripts_);
  visitor->Trace(pending_async_scripts_);
  visitor->Trace(async_scripts_to_execute_soon_);
  visitor->Trace(in_order_scripts_to_execute_soon_);
  visitor->Trace(currently_streamed_script_);
  visitor->Trace(streamers_);
}

}  // namespace blink
