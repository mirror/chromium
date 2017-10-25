// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/devtools/devtools_agent.h"

#include <stddef.h>

#include <map>
#include <utility>

#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sequence_checker.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "content/common/devtools_messages.h"
#include "content/common/frame_messages.h"
#include "content/public/common/manifest.h"
#include "content/renderer/devtools/devtools_cpu_throttler.h"
#include "content/renderer/manifest/manifest_manager.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_widget.h"
#include "ipc/ipc_channel.h"
#include "third_party/WebKit/public/platform/WebFloatRect.h"
#include "third_party/WebKit/public/platform/WebPoint.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebDevToolsAgent.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

using blink::WebDevToolsAgent;
using blink::WebDevToolsAgentClient;
using blink::WebLocalFrame;
using blink::WebPoint;
using blink::WebString;

using base::trace_event::TraceLog;

namespace content {

namespace {

const size_t kMaxMessageChunkSize = IPC::Channel::kMaximumMessageSize / 4;
const char kPageGetAppManifest[] = "Page.getAppManifest";

class WebKitClientMessageLoopImpl
    : public WebDevToolsAgentClient::WebKitClientMessageLoop {
 public:
  WebKitClientMessageLoopImpl() = default;
  ~WebKitClientMessageLoopImpl() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }
  void Run() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    base::RunLoop* const previous_run_loop = run_loop_;
    base::RunLoop run_loop;
    run_loop_ = &run_loop;

    base::MessageLoop::ScopedNestableTaskAllower allow(
        base::MessageLoop::current());
    run_loop.Run();

    run_loop_ = previous_run_loop;
  }
  void QuitNow() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(run_loop_);

    run_loop_->Quit();
  }

 private:
  base::RunLoop* run_loop_ = nullptr;

  SEQUENCE_CHECKER(sequence_checker_);
};

} //  namespace

class DevToolsAgent::SessionProxy : public mojom::DevToolsSession {
 public:
  SessionProxy(int session_id, scoped_refptr<SingleThreadTaskRunner> agent_task_runner, base::WeakPtr<DevToolsAgent> agent)
      : session_id_(session_id),
        agent_task_runner_(agent_task_runner),
        agent_(agent),
        binding_(this) {}
  ~Session() {}

  void BindInterface(mojom::DevToolsSessionRequest session_request) {
    binding_.Bind(session_request);
  }

  // mojom::DevToolsSession implementation.
  void DispatchProtocolMessage(int call_id, const std::string& method, const std::string& message) {
    agent_task_runner_->PosTask(base::Bind(&DevToolsAgent::DispatchOnInspectorBackend, agent_, session_id_, call_id, method, message));
  }

  void InspectElement(const gfx::Point& point) {
    agent_task_runner_->PosTask(base::Bind(&DevToolsAgent::InspectElement, agent_, session_id_, point));
  }

 private:
  int session_id_;
  scoped_refptr<SingleThreadTaskRunner> agent_task_runner_;
  base::WeakPtr<DevToolsAgent> agent_;
  mojo::Binding<mojom::DevToolsSession> binding_;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

DevToolsAgent::DevToolsAgent(RenderFrameImpl* frame)
    : RenderFrameObserver(frame),
      paused_(false),
      frame_(frame),
      weak_factory_(this) {
  frame_->GetWebFrame()->SetDevToolsAgentClient(this);
}

DevToolsAgent::~DevToolsAgent() {
}

void DevToolsAgent::WidgetWillClose() {
  ContinueProgram();
}

void DevToolsAgent::OnDestruct() {
  delete this;
}

void DevToolsAgent::AttachDevToolsSession(
    mojom::DevToolsSessionHostPtr session_host,
    const base::Optional<const std::string&> reattach_state,
    mojom::DevToolsSessionRequest session_request) {
  int session_id = ++last_session_id_;
  if (reattach_state.has_value())
    GetWebAgent()->Reattach(session_id, WebString::FromUTF8(agent_state));
  else
    GetWebAgent()->Attach(session_id);
  SessionProxy* proxy = new SessionProxy(session_id, ThreadTaskRunner::Get(), weak_factory_.GetWeakPtr());
  session_proxies_[session_id] = proxy;
  session_hosts_[session_id] = session_host;
  ChildProcess::current()->io_task_runner()->PostTask(
      base::Bind(&DevToolsAgent::SessionProxy::BindInterface,
                 base::Unretained(proxy),
                 std::move(session_request)));
  session_host.set_connection_error_handler(base::BindOnce(
      &DevToolsAgent::Detach, base::Unretained(this), session_id));
}

void DevToolsAgent::DetachSession(int session_id) {
  GetWebAgent()->Detach(session_id);
  session_proxies_.erase(session_id);
  session_hosts_.erase(session_id);
}

void DevToolsAgent::SendProtocolMessage(int session_id,
                                        int call_id,
                                        const blink::WebString& message,
                                        const blink::WebString& state_cookie) {
  if (!send_protocol_message_callback_for_test_.is_null()) {
    send_protocol_message_callback_for_test_.Run(
        session_id, call_id, message.Utf8(), state_cookie.Utf8());
    return;
  }

  auto it = session_hosts_.find(session_id);
  if (it == session_hosts_.end())
    return;

  mojom::DevToolsMessageChunkPtr chunk = mojom::DevToolsMessageChunk::New();
  chunk->message_size = message.size();
  chunk->is_first = true;

  if (message.length() < kMaxMessageChunkSize) {
    chunk->data = message;
    chunk->call_id = call_id;
    chunk->post_state = post_state;
    chunk->is_last = true;
    it->second->DispatchProtocolMessage(chunk);
    return;
  }

  for (size_t pos = 0; pos < message.length(); pos += kMaxMessageChunkSize) {
    chunk->is_last = pos + kMaxMessageChunkSize >= message.length();
    chunk->call_id = chunk->is_last ? call_id : 0;
    chunk->post_state = chunk->is_last ? post_state : std::string();
    chunk->data = message.substr(pos, kMaxMessageChunkSize);
    it->second->DispatchProtocolMessage(chunk);
    chunk->is_first = false;
    chunk->message_size = 0;
  }
}

// static
blink::WebDevToolsAgentClient::WebKitClientMessageLoop*
DevToolsAgent::createMessageLoopWrapper() {
  return new WebKitClientMessageLoopImpl();
}

blink::WebDevToolsAgentClient::WebKitClientMessageLoop*
DevToolsAgent::CreateClientMessageLoop() {
  return createMessageLoopWrapper();
}

void DevToolsAgent::WillEnterDebugLoop() {
  paused_ = true;
}

void DevToolsAgent::DidExitDebugLoop() {
  paused_ = false;
}

bool DevToolsAgent::RequestDevToolsForFrame(
    int session_id,
    blink::WebLocalFrame* webFrame) {
  RenderFrameImpl* frame = RenderFrameImpl::FromWebFrame(webFrame);
  if (!frame)
    return false;
  auto it = session_hosts_.find(session_id);
  if (it == session_hosts_.end())
    return false;
  it->second->RequestNewWindow(frame->GetRoutingID(), base::Bind(&DevToolsAgent::OnRequestNewWindowACK, weak_factory_.GetWeakPtr(), session_id));
  return true;
}

void DevToolsAgent::EnableTracing(const WebString& category_filter) {
  // Tracing is already started by DevTools TracingHandler::Start for the
  // renderer target in the browser process. It will eventually start tracing in
  // the renderer process via IPC. But we still need a redundant
  // TraceLog::SetEnabled call here for
  // InspectorTracingAgent::emitMetadataEvents(), at which point, we are not
  // sure if tracing is already started in the renderer process.
  TraceLog* trace_log = TraceLog::GetInstance();
  trace_log->SetEnabled(
      base::trace_event::TraceConfig(category_filter.Utf8(), ""),
      TraceLog::RECORDING_MODE);
}

void DevToolsAgent::DisableTracing() {
  TraceLog::GetInstance()->SetDisabled();
}

void DevToolsAgent::SetCPUThrottlingRate(double rate) {
  DevToolsCPUThrottler::GetInstance()->SetThrottlingRate(rate);
}

void DevToolsAgent::DispatchOnInspectorBackend(int session_id,
                                               int call_id,
                                               const std::string& method,
                                               const std::string& message) {
  TRACE_EVENT0("devtools", "DevToolsAgent::DispatchOnInspectorBackend");
  if (method == kPageGetAppManifest) {
    ManifestManager* manager = frame_->manifest_manager();
    manager->GetManifest(base::BindOnce(&DevToolsAgent::GotManifest,
                                        weak_factory_.GetWeakPtr(), session_id,
                                        call_id));
    return;
  }
  GetWebAgent()->DispatchOnInspectorBackend(session_id, call_id,
                                            WebString::FromUTF8(method),
                                            WebString::FromUTF8(message));
}

void DevToolsAgent::InspectElement(int session_id, const gfx::Point& point) {
  blink::WebFloatRect point_rect(point.x(), point.y(), 0, 0);
  frame_->GetRenderWidget()->ConvertWindowToViewport(&point_rect);
  GetWebAgent()->InspectElementAt(session_id,
                                  WebPoint(point_rect.x, point_rect.y));
}

void DevToolsAgent::OnRequestNewWindowACK(int session_id, bool success) {
  if (!success)
    GetWebAgent()->FailedToRequestDevTools(session_id);
}

void DevToolsAgent::ContinueProgram() {
  GetWebAgent()->ContinueProgram();
}

WebDevToolsAgent* DevToolsAgent::GetWebAgent() {
  return frame_->GetWebFrame()->DevToolsAgent();
}

bool DevToolsAgent::IsAttached() {
  return !!session_proxies_.size();
}

void DevToolsAgent::DetachAllSessions() {
  for (auto& it : session_proxies_)
    GetWebAgent()->Detach(it.first);
  session_proxies_.clear();
  session_hosts_.clear();
}

void DevToolsAgent::GotManifest(int session_id,
                                int call_id,
                                const GURL& manifest_url,
                                const Manifest& manifest,
                                const ManifestDebugInfo& debug_info) {
  if (session_proxies_.find(session_id) == session_proxies_.end())
    return;

  std::unique_ptr<base::DictionaryValue> response(new base::DictionaryValue());
  response->SetInteger("id", call_id);
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  std::unique_ptr<base::ListValue> errors(new base::ListValue());

  bool failed = false;
  for (const auto& error : debug_info.errors) {
    std::unique_ptr<base::DictionaryValue> error_value(
        new base::DictionaryValue());
    error_value->SetString("message", error.message);
    error_value->SetBoolean("critical", error.critical);
    error_value->SetInteger("line", error.line);
    error_value->SetInteger("column", error.column);
    if (error.critical)
      failed = true;
    errors->Append(std::move(error_value));
  }

  WebString url =
      frame_->GetWebFrame()->GetDocument().ManifestURL().GetString();
  result->SetString("url", url.Utf16());
  if (!failed)
    result->SetString("data", debug_info.raw_data);
  result->Set("errors", std::move(errors));
  response->Set("result", std::move(result));

  std::string json_message;
  base::JSONWriter::Write(*response, &json_message);
  SendProtocolMessage(session_id, call_id, json_message, std::string());
}

}  // namespace content
