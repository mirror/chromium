/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "core/inspector/InspectorWorkerAgent.h"

#include "core/dom/Document.h"
#include "core/inspector/InspectedFrames.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

using protocol::Maybe;
using protocol::Response;

namespace WorkerAgentState {
static const char kAutoAttach[] = "autoAttach";
static const char kSubscribe[] = "subscribe";
static const char kWaitForDebuggerOnStart[] = "waitForDebuggerOnStart";
static const char kAttachedSessionIds[] = "attachedSessionIds";
};

namespace {

std::unique_ptr<protocol::Target::TargetInfo> CreateInfo(
    WorkerInspectorProxy* proxy) {
  return protocol::Target::TargetInfo::create()
      .setTargetId(proxy->InspectorId())
      .setType("worker")
      .setTitle(proxy->Url())
      .setUrl(proxy->Url())
      .setAttached(proxy->HasConnectedInspector())
      .build();
}

}  // namespace

int InspectorWorkerAgent::s_last_connection_ = 0;

InspectorWorkerAgent::InspectorWorkerAgent(InspectedFrames* inspected_frames)
    : inspected_frames_(inspected_frames) {}

InspectorWorkerAgent::~InspectorWorkerAgent() {}

void InspectorWorkerAgent::Restore() {
  protocol::DictionaryValue* attached = AttachedSessionIds();
  for (size_t i = 0; i < attached->size(); ++i)
    GetFrontend()->detachedFromTarget(attached->at(i).first);
  state_->remove(WorkerAgentState::kAttachedSessionIds);

  InstrumentIfNeeded();
  if (!instrumenting_)
    return;
  for (WorkerInspectorProxy* proxy : WorkerInspectorProxy::AllProxies()) {
    // For now we assume this is document. TODO(kinuko): Fix this.
    DCHECK(proxy->GetExecutionContext()->IsDocument());
    Document* document = ToDocument(proxy->GetExecutionContext());
    if (!document->GetFrame() ||
        !inspected_frames_->Contains(document->GetFrame())) {
      continue;
    }

    if (SubscriptionEnabled())
      GetFrontend()->targetCreated(CreateInfo(proxy));
    if (AutoAttachEnabled())
      Connect(proxy, false);
  }
}

Response InspectorWorkerAgent::disable() {
  DisconnectFromProxy(nullptr, false);
  if (instrumenting_) {
    instrumenting_agents_->removeInspectorWorkerAgent(this);
    instrumenting_ = false;
  }
  state_->setBoolean(WorkerAgentState::kSubscribe, false);
  state_->setBoolean(WorkerAgentState::kAutoAttach, false);
  state_->setBoolean(WorkerAgentState::kWaitForDebuggerOnStart, false);
  state_->remove(WorkerAgentState::kAttachedSessionIds);
  return Response::OK();
}

Response InspectorWorkerAgent::getWorkers(
    bool subscribe,
    bool auto_attach,
    bool wait_for_debugger_on_start,
    std::unique_ptr<protocol::Array<protocol::Target::TargetInfo>>* workers) {
  state_->setBoolean(WorkerAgentState::kSubscribe, subscribe);
  state_->setBoolean(WorkerAgentState::kAutoAttach, auto_attach);
  state_->setBoolean(WorkerAgentState::kWaitForDebuggerOnStart,
                     wait_for_debugger_on_start);
  InstrumentIfNeeded();

  *workers = protocol::Array<protocol::Target::TargetInfo>::create();
  for (WorkerInspectorProxy* proxy : WorkerInspectorProxy::AllProxies()) {
    // For now we assume this is document. TODO(kinuko): Fix this.
    DCHECK(proxy->GetExecutionContext()->IsDocument());
    Document* document = ToDocument(proxy->GetExecutionContext());
    if (!document->GetFrame() ||
        !inspected_frames_->Contains(document->GetFrame())) {
      continue;
    }
    (*workers)->addItem(CreateInfo(proxy));
  }
  return Response::OK();
}

Response InspectorWorkerAgent::setAttachToFrames(bool attach) {
  return Response::OK();
}

Response InspectorWorkerAgent::setAutoAttach(bool auto_attach,
                                             bool wait_for_debugger_on_start) {
  return Response::OK();
}

bool InspectorWorkerAgent::AutoAttachEnabled() {
  return state_->booleanProperty(WorkerAgentState::kAutoAttach, false);
}

bool InspectorWorkerAgent::SubscriptionEnabled() {
  return state_->booleanProperty(WorkerAgentState::kSubscribe, false);
}

Response InspectorWorkerAgent::sendMessageToTarget(const String& message,
                                                   Maybe<String> session_id,
                                                   Maybe<String> target_id) {
  int connection_id;
  Response response = FindConnection(std::move(session_id),
                                     std::move(target_id), &connection_id);
  if (!response.isSuccess())
    return response;
  WorkerInspectorProxy* proxy = connected_proxies_.at(connection_id);
  proxy->SendMessageToInspector(connection_id, message);
  return Response::OK();
}

Response InspectorWorkerAgent::attachToTarget(const String& target_id,
                                              String* out_session_id) {
  for (WorkerInspectorProxy* proxy : WorkerInspectorProxy::AllProxies()) {
    if (proxy->InspectorId() == target_id) {
      *out_session_id = Connect(proxy, false);
      return Response::OK();
    }
  }
  return Response::Error("No target with given id");
}

Response InspectorWorkerAgent::detachFromTarget(Maybe<String> session_id,
                                                Maybe<String> target_id) {
  int connection_id;
  Response response = FindConnection(std::move(session_id),
                                     std::move(target_id), &connection_id);
  if (!response.isSuccess())
    return response;
  Disconnect(connection_id, true);
  return Response::OK();
}

void InspectorWorkerAgent::SetHostId(const String& host_id) {
  host_id_ = host_id;
}

void InspectorWorkerAgent::SetTracingSessionId(
    const String& tracing_session_id) {
  tracing_session_id_ = tracing_session_id;
  if (tracing_session_id.IsEmpty())
    return;
  for (auto& id_proxy : connected_proxies_)
    id_proxy.value->WriteTimelineStartedEvent(tracing_session_id);
}

void InspectorWorkerAgent::ShouldWaitForDebuggerOnWorkerStart(bool* result) {
  if (AutoAttachEnabled() &&
      state_->booleanProperty(WorkerAgentState::kWaitForDebuggerOnStart, false))
    *result = true;
}

void InspectorWorkerAgent::DidStartWorker(WorkerInspectorProxy* proxy,
                                          bool waiting_for_debugger) {
  if (SubscriptionEnabled())
    GetFrontend()->targetCreated(CreateInfo(proxy));
  if (AutoAttachEnabled()) {
    Connect(proxy, waiting_for_debugger);
    if (!tracing_session_id_.IsEmpty())
      proxy->WriteTimelineStartedEvent(tracing_session_id_);
  }
}

void InspectorWorkerAgent::WorkerTerminated(WorkerInspectorProxy* proxy) {
  DisconnectFromProxy(proxy, true);
  if (SubscriptionEnabled())
    GetFrontend()->targetDestroyed(proxy->InspectorId());
}

void InspectorWorkerAgent::DisconnectFromProxy(WorkerInspectorProxy* proxy,
                                               bool report_to_frontend) {
  Vector<int> connection_ids;
  for (auto& it : connected_proxies_) {
    if (!proxy || it.value == proxy)
      connection_ids.push_back(it.key);
  }
  for (int connection_id : connection_ids)
    Disconnect(connection_id, true);
}

void InspectorWorkerAgent::DidCommitLoadForLocalFrame(LocalFrame* frame) {
  if (!AutoAttachEnabled() || frame != inspected_frames_->Root())
    return;

  // During navigation workers from old page may die after a while.
  // Usually, it's fine to report them terminated later, but some tests
  // expect strict set of workers, and we reuse renderer between tests.
  DisconnectFromProxy(nullptr, true);
}

Response InspectorWorkerAgent::FindConnection(Maybe<String> session_id,
                                              Maybe<String> target_id,
                                              int* connection_id) {
  if (session_id.isJust()) {
    auto it = session_id_to_connection_.find(session_id.fromJust());
    if (it == session_id_to_connection_.end())
      return Response::Error("No session with given id");
    *connection_id = it->value;
    return Response::OK();
  }
  if (target_id.isJust()) {
    *connection_id = 0;
    for (auto& it : connected_proxies_) {
      if (it.value->InspectorId() == target_id.fromJust()) {
        if (*connection_id)
          return Response::Error("Multiple sessions attached, specify id");
        *connection_id = it.key;
      }
    }
    if (!*connection_id)
      return Response::Error("No target with given id");
    return Response::OK();
  }
  return Response::Error("Session id must be specified");
}

protocol::DictionaryValue* InspectorWorkerAgent::AttachedSessionIds() {
  protocol::DictionaryValue* ids =
      state_->getObject(WorkerAgentState::kAttachedSessionIds);
  if (!ids) {
    std::unique_ptr<protocol::DictionaryValue> new_ids =
        protocol::DictionaryValue::create();
    ids = new_ids.get();
    state_->setObject(WorkerAgentState::kAttachedSessionIds,
                      std::move(new_ids));
  }
  return ids;
}

void InspectorWorkerAgent::InstrumentIfNeeded() {
  bool need_instrumentation = AutoAttachEnabled() || SubscriptionEnabled();
  if (need_instrumentation && !instrumenting_)
    instrumenting_agents_->addInspectorWorkerAgent(this);
  else if (!need_instrumentation && instrumenting_)
    instrumenting_agents_->removeInspectorWorkerAgent(this);
  instrumenting_ = need_instrumentation;
}

String InspectorWorkerAgent::Connect(WorkerInspectorProxy* proxy,
                                     bool waiting_for_debugger) {
  int connection = ++s_last_connection_;
  connected_proxies_.Set(connection, proxy);

  String session_id = proxy->InspectorId() + "-" + String::Number(connection);
  session_id_to_connection_.Set(session_id, connection);
  connection_to_session_id_.Set(connection, session_id);

  proxy->ConnectToInspector(connection, host_id_, this);
  DCHECK(GetFrontend());
  AttachedSessionIds()->setBoolean(session_id, true);
  GetFrontend()->attachedToTarget(session_id, CreateInfo(proxy),
                                  waiting_for_debugger);
  return session_id;
}

void InspectorWorkerAgent::Disconnect(int connection_id,
                                      bool report_to_frontend) {
  String session_id = connection_to_session_id_.at(connection_id);
  WorkerInspectorProxy* proxy = connected_proxies_.at(connection_id);
  if (report_to_frontend) {
    AttachedSessionIds()->remove(session_id);
    GetFrontend()->detachedFromTarget(session_id, proxy->InspectorId());
  }
  proxy->DisconnectFromInspector(connection_id, this);
  connection_to_session_id_.erase(connection_id);
  session_id_to_connection_.erase(session_id);
  connected_proxies_.erase(connection_id);
}

void InspectorWorkerAgent::DispatchMessageFromWorker(
    WorkerInspectorProxy* proxy,
    int connection,
    const String& message) {
  auto it = connection_to_session_id_.find(connection);
  if (it == connection_to_session_id_.end())
    return;
  GetFrontend()->receivedMessageFromTarget(it->value, message,
                                           proxy->InspectorId());
}

DEFINE_TRACE(InspectorWorkerAgent) {
  visitor->Trace(connected_proxies_);
  visitor->Trace(inspected_frames_);
  InspectorBaseAgent::Trace(visitor);
}

}  // namespace blink
