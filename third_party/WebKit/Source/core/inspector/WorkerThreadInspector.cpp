/*
 * Copyright (c) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/inspector/WorkerThreadInspector.h"

#include "bindings/core/v8/SourceLocation.h"
#include "bindings/core/v8/V8ErrorHandler.h"
#include "bindings/core/v8/V8ScriptRunner.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "core/events/ErrorEvent.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/inspector/ConsoleMessageStorage.h"
#include "core/inspector/IdentifiersFactory.h"
#include "core/inspector/V8InspectorString.h"
#include "core/workers/ThreadedWorkletGlobalScope.h"
#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerReportingProxy.h"
#include "core/workers/WorkerThread.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "v8/include/v8.h"

namespace blink {

namespace {

const int kInvalidContextGroupId = 0;

}  // namespace

WorkerThreadInspector* WorkerThreadInspector::From(v8::Isolate* isolate) {
  ThreadInspector* inspector = ThreadInspector::From(isolate);
  if (!inspector)
    return nullptr;
  DCHECK(inspector->IsWorker());
  return static_cast<WorkerThreadInspector*>(inspector);
}

WorkerThreadInspector::WorkerThreadInspector(v8::Isolate* isolate)
    : ThreadInspector(isolate),
      paused_context_group_id_(kInvalidContextGroupId) {}

WorkerThreadInspector::~WorkerThreadInspector() {
  DCHECK(worker_threads_.IsEmpty());
}

void WorkerThreadInspector::ReportConsoleMessage(ExecutionContext* context,
                                                 MessageSource source,
                                                 MessageLevel level,
                                                 const String& message,
                                                 SourceLocation* location) {
  if (!context)
    return;
  ToWorkerOrWorkletGlobalScope(context)
      ->GetThread()
      ->GetWorkerReportingProxy()
      .ReportConsoleMessage(source, level, message, location);
}

int WorkerThreadInspector::ContextGroupId(WorkerThread* worker_thread) {
  return worker_thread->GetWorkerThreadId();
}

void WorkerThreadInspector::ContextCreated(WorkerThread* worker_thread,
                                           v8::Local<v8::Context> context) {
  int worker_context_group_id = ContextGroupId(worker_thread);
  v8_inspector::V8ContextInfo context_info(context, worker_context_group_id,
                                           v8_inspector::StringView());
  String origin = worker_thread->GlobalScope()->Url().GetString();
  context_info.origin = ToV8InspectorStringView(origin);
  GetV8Inspector()->contextCreated(context_info);

  DCHECK(!worker_threads_.Contains(worker_context_group_id));
  worker_threads_.insert(worker_context_group_id, worker_thread);
}

void WorkerThreadInspector::ContextWillBeDestroyed(
    WorkerThread* worker_thread,
    v8::Local<v8::Context> context) {
  int worker_context_group_id = ContextGroupId(worker_thread);
  DCHECK(worker_threads_.Contains(worker_context_group_id));
  worker_threads_.erase(worker_context_group_id);
  GetV8Inspector()->contextDestroyed(context);
}

void WorkerThreadInspector::ExceptionThrown(WorkerThread* worker_thread,
                                            ErrorEvent* event) {
  worker_thread->GetWorkerReportingProxy().ReportConsoleMessage(
      kJSMessageSource, kErrorMessageLevel, event->MessageForConsole(),
      event->Location());

  const String default_message = "Uncaught";
  ScriptState* script_state =
      worker_thread->GlobalScope()->ScriptController()->GetScriptState();
  if (script_state && script_state->ContextIsValid()) {
    ScriptState::Scope scope(script_state);
    v8::Local<v8::Value> exception =
        V8ErrorHandler::LoadExceptionFromErrorEventWrapper(
            script_state, event, script_state->GetContext()->Global());
    SourceLocation* location = event->Location();
    String message = event->MessageForConsole();
    String url = location->Url();
    GetV8Inspector()->exceptionThrown(
        script_state->GetContext(), ToV8InspectorStringView(default_message),
        exception, ToV8InspectorStringView(message),
        ToV8InspectorStringView(url), location->LineNumber(),
        location->ColumnNumber(), location->TakeStackTrace(),
        location->ScriptId());
  }
}

int WorkerThreadInspector::ContextGroupId(ExecutionContext* context) {
  return ContextGroupId(ToWorkerOrWorkletGlobalScope(context)->GetThread());
}

void WorkerThreadInspector::runMessageLoopOnPause(int context_group_id) {
  DCHECK_EQ(kInvalidContextGroupId, paused_context_group_id_);
  DCHECK(worker_threads_.Contains(context_group_id));
  paused_context_group_id_ = context_group_id;
  worker_threads_.at(context_group_id)
      ->StartRunningDebuggerTasksOnPauseOnWorkerThread();
}

void WorkerThreadInspector::quitMessageLoopOnPause() {
  DCHECK_NE(kInvalidContextGroupId, paused_context_group_id_);
  DCHECK(worker_threads_.Contains(paused_context_group_id_));
  worker_threads_.at(paused_context_group_id_)
      ->StopRunningDebuggerTasksOnPauseOnWorkerThread();
  paused_context_group_id_ = kInvalidContextGroupId;
}

void WorkerThreadInspector::muteMetrics(int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
}

void WorkerThreadInspector::unmuteMetrics(int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
}

v8::Local<v8::Context> WorkerThreadInspector::ensureDefaultContextInGroup(
    int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
  ScriptState* script_state = worker_threads_.at(context_group_id)
                                  ->GlobalScope()
                                  ->ScriptController()
                                  ->GetScriptState();
  return script_state ? script_state->GetContext() : v8::Local<v8::Context>();
}

void WorkerThreadInspector::beginEnsureAllContextsInGroup(
    int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
}

void WorkerThreadInspector::endEnsureAllContextsInGroup(int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
}

bool WorkerThreadInspector::canExecuteScripts(int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
  return true;
}

void WorkerThreadInspector::runIfWaitingForDebugger(int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
  worker_threads_.at(context_group_id)
      ->StopRunningDebuggerTasksOnPauseOnWorkerThread();
}

void WorkerThreadInspector::consoleAPIMessage(
    int context_group_id,
    v8::Isolate::MessageErrorLevel level,
    const v8_inspector::StringView& message,
    const v8_inspector::StringView& url,
    unsigned line_number,
    unsigned column_number,
    v8_inspector::V8StackTrace* stack_trace) {
  DCHECK(worker_threads_.Contains(context_group_id));
  WorkerThread* worker_thread = worker_threads_.at(context_group_id);
  std::unique_ptr<SourceLocation> location =
      SourceLocation::Create(ToCoreString(url), line_number, column_number,
                             stack_trace ? stack_trace->clone() : nullptr, 0);
  worker_thread->GetWorkerReportingProxy().ReportConsoleMessage(
      kConsoleAPIMessageSource, V8MessageLevelToMessageLevel(level),
      ToCoreString(message), location.get());
}

void WorkerThreadInspector::consoleClear(int context_group_id) {
  DCHECK(worker_threads_.Contains(context_group_id));
  WorkerThread* worker_thread = worker_threads_.at(context_group_id);
  worker_thread->GetConsoleMessageStorage()->Clear();
}

v8::MaybeLocal<v8::Value> WorkerThreadInspector::memoryInfo(
    v8::Isolate*,
    v8::Local<v8::Context>) {
  NOTREACHED();
  return v8::MaybeLocal<v8::Value>();
}

}  // namespace blink
