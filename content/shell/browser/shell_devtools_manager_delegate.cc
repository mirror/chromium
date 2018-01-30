// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_devtools_manager_delegate.h"

#include <stdint.h>

#include <vector>

#include "base/atomicops.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "content/shell/browser/layout_test/secondary_test_window_observer.h"
#include "content/shell/browser/shell.h"
#include "content/shell/common/shell_content_client.h"
#include "content/shell/common/shell_switches.h"
#include "content/shell/grit/shell_resources.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/socket/tcp_server_socket.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(OS_ANDROID)
#include "content/public/browser/devtools_frontend_host.h"
#endif

#if defined(OS_ANDROID)
#include "content/public/browser/android/devtools_auth.h"
#include "net/socket/unix_domain_server_socket_posix.h"
#endif

namespace content {

namespace {

#if defined(OS_ANDROID)
const char kFrontEndURL[] =
    "http://chrome-devtools-frontend.appspot.com/serve_rev/%s/inspector.html";
#endif

const int kBackLog = 10;

base::subtle::Atomic32 g_last_used_port;

#if defined(OS_ANDROID)
class UnixDomainServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  explicit UnixDomainServerSocketFactory(const std::string& socket_name)
      : socket_name_(socket_name) {}

 private:
  // content::DevToolsSocketFactory.
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    std::unique_ptr<net::UnixDomainServerSocket> socket(
        new net::UnixDomainServerSocket(base::Bind(&CanUserConnectToDevTools),
                                        true /* use_abstract_namespace */));
    if (socket->BindAndListen(socket_name_, kBackLog) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    return std::move(socket);
  }

  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* out_name) override {
    return nullptr;
  }

  std::string socket_name_;

  DISALLOW_COPY_AND_ASSIGN(UnixDomainServerSocketFactory);
};
#else
class TCPServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  TCPServerSocketFactory(const std::string& address, uint16_t port)
      : address_(address), port_(port) {}

 private:
  // content::DevToolsSocketFactory.
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    std::unique_ptr<net::ServerSocket> socket(
        new net::TCPServerSocket(nullptr, net::NetLogSource()));
    if (socket->ListenWithAddressAndPort(address_, port_, kBackLog) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    net::IPEndPoint endpoint;
    if (socket->GetLocalAddress(&endpoint) == net::OK)
      base::subtle::NoBarrier_Store(&g_last_used_port, endpoint.port());

    return socket;
  }

  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* out_name) override {
    return nullptr;
  }

  std::string address_;
  uint16_t port_;

  DISALLOW_COPY_AND_ASSIGN(TCPServerSocketFactory);
};
#endif

std::unique_ptr<content::DevToolsSocketFactory> CreateSocketFactory() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
#if defined(OS_ANDROID)
  std::string socket_name = "content_shell_devtools_remote";
  if (command_line.HasSwitch(switches::kRemoteDebuggingSocketName)) {
    socket_name = command_line.GetSwitchValueASCII(
        switches::kRemoteDebuggingSocketName);
  }
  return std::unique_ptr<content::DevToolsSocketFactory>(
      new UnixDomainServerSocketFactory(socket_name));
#else
  // See if the user specified a port on the command line (useful for
  // automation). If not, use an ephemeral port by specifying 0.
  uint16_t port = 0;
  if (command_line.HasSwitch(switches::kRemoteDebuggingPort)) {
    int temp_port;
    std::string port_str =
        command_line.GetSwitchValueASCII(switches::kRemoteDebuggingPort);
    if (base::StringToInt(port_str, &temp_port) &&
        temp_port >= 0 && temp_port < 65535) {
      port = static_cast<uint16_t>(temp_port);
    } else {
      DLOG(WARNING) << "Invalid http debugger port number " << temp_port;
    }
  }
  return std::unique_ptr<content::DevToolsSocketFactory>(
      new TCPServerSocketFactory("127.0.0.1", port));
#endif
}

const char kIdParam[] = "id";
const char kMethodParam[] = "method";
const char kParamsParam[] = "params";
const char kResultParam[] = "result";

const char kBrowserGetWindowForTarget[] = "Browser.getWindowForTarget";
const char kBrowserSetWindowBounds[] = "Browser.setWindowBounds";
const int kServerError = -32000;
const int kDefaultWindowId = 1;

void BuildError(int id, const std::string& message, std::string* output) {
  base::Value error_value(base::Value::Type::DICTIONARY);
  error_value.SetKey("message", base::Value(message));
  error_value.SetKey("code", base::Value(kServerError));
  base::Value message_value(base::Value::Type::DICTIONARY);
  message_value.SetKey("error", std::move(error_value));
  message_value.SetKey(kIdParam, base::Value(id));
  base::JSONWriter::Write(message_value, output);
}

Shell* ShellForDevToolsAgentHost(DevToolsAgentHost* agent_host) {
  WebContents* web_contents = agent_host->GetWebContents();
  RenderViewHost* render_view_host =
      web_contents ? web_contents->GetRenderViewHost() : nullptr;
  return render_view_host ? Shell::FromRenderViewHost(render_view_host)
                          : nullptr;
}
} //  namespace

// ShellDevToolsManagerDelegate ----------------------------------------------

// static
int ShellDevToolsManagerDelegate::GetHttpHandlerPort() {
  return base::subtle::NoBarrier_Load(&g_last_used_port);
}

// static
void ShellDevToolsManagerDelegate::StartHttpHandler(
    BrowserContext* browser_context) {
  std::string frontend_url;
#if defined(OS_ANDROID)
  frontend_url = base::StringPrintf(kFrontEndURL, GetWebKitRevision().c_str());
#endif
  DevToolsAgentHost::StartRemoteDebuggingServer(
      CreateSocketFactory(), frontend_url, browser_context->GetPath(),
      base::FilePath());

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kRemoteDebuggingPipe))
    DevToolsAgentHost::StartRemoteDebuggingPipeHandler();
}

// static
void ShellDevToolsManagerDelegate::StopHttpHandler() {
  DevToolsAgentHost::StopRemoteDebuggingServer();
}

ShellDevToolsManagerDelegate::ShellDevToolsManagerDelegate(
    BrowserContext* browser_context)
    : browser_context_(browser_context) {
}

ShellDevToolsManagerDelegate::~ShellDevToolsManagerDelegate() {
}

scoped_refptr<DevToolsAgentHost>
ShellDevToolsManagerDelegate::CreateNewTarget(const GURL& url) {
  Shell* shell = Shell::CreateNewWindow(browser_context_,
                                        url,
                                        nullptr,
                                        gfx::Size());
  if (switches::IsRunLayoutTestSwitchPresent())
    SecondaryTestWindowObserver::CreateForWebContents(shell->web_contents());
  return DevToolsAgentHost::GetOrCreateFor(shell->web_contents());
}

std::string ShellDevToolsManagerDelegate::GetDiscoveryPageHTML() {
#if defined(OS_ANDROID)
  return std::string();
#else
  return ui::ResourceBundle::GetSharedInstance()
      .GetRawDataResource(IDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE)
      .as_string();
#endif
}

std::string ShellDevToolsManagerDelegate::GetFrontendResource(
    const std::string& path) {
#if defined(OS_ANDROID)
  return std::string();
#else
  return content::DevToolsFrontendHost::GetFrontendResource(path).as_string();
#endif
}

bool ShellDevToolsManagerDelegate::HandleCommand(
    DevToolsAgentHost* agent_host,
    DevToolsAgentHostClient* client,
    base::DictionaryValue* command_dict) {
  if (!command_dict)
    return false;

  base::Value* id =
      command_dict->FindKeyOfType(kIdParam, base::Value::Type::INTEGER);
  if (!id || id->GetInt() < 0)
    return false;

  base::Value* method_value =
      command_dict->FindKeyOfType(kMethodParam, base::Value::Type::STRING);
  if (!method_value)
    return false;

  std::string method = method_value->GetString();
  if (method != kBrowserGetWindowForTarget && method != kBrowserSetWindowBounds)
    return false;

  std::string message;
  Shell* shell = ShellForDevToolsAgentHost(agent_host);
  if (!shell) {
    BuildError(id->GetInt(), "No shell in the target", &message);
    client->DispatchProtocolMessage(agent_host, message);
    return true;
  }

  base::Value* params =
      command_dict->FindKeyOfType(kParamsParam, base::Value::Type::DICTIONARY);
  if (method == kBrowserGetWindowForTarget) {
    base::Value* target_id =
        params ? params->FindKeyOfType("targetId", base::Value::Type::STRING)
               : nullptr;
    if (!target_id || agent_host->GetId() != target_id->GetString()) {
      BuildError(id->GetInt(), "Invalid targetId", &message);
      client->DispatchProtocolMessage(agent_host, message);
      return true;
    }

    gfx::Rect rect = shell->GetRestoredBounds();
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetKey(kIdParam, base::Value(id->GetInt()));

    base::Value bounds(base::Value::Type::DICTIONARY);
    bounds.SetKey("left", base::Value(rect.x()));
    bounds.SetKey("right", base::Value(rect.right()));
    bounds.SetKey("top", base::Value(rect.y()));
    bounds.SetKey("bottom", base::Value(rect.bottom()));

    base::Value result(base::Value::Type::DICTIONARY);
    result.SetKey("bounds", std::move(bounds));
    result.SetKey("windowId", base::Value(kDefaultWindowId));
    dict.SetKey(kResultParam, std::move(result));

    base::JSONWriter::Write(dict, &message);

    client->DispatchProtocolMessage(agent_host, message);
    return true;
  } else if (method == kBrowserSetWindowBounds) {
    base::Value* window_id =
        params ? params->FindKeyOfType("windowId", base::Value::Type::INTEGER)
               : nullptr;
    if (!window_id || window_id->GetInt() != kDefaultWindowId) {
      BuildError(id->GetInt(), "Invalid windowId", &message);
      client->DispatchProtocolMessage(agent_host, message);
      return true;
    }
    base::Value* bounds =
        params->FindKeyOfType("bounds", base::Value::Type::DICTIONARY);
    if (!bounds) {
      BuildError(id->GetInt(), "Invalid bounds", &message);
      client->DispatchProtocolMessage(agent_host, message);
      return true;
    }
    base::Value* left =
        bounds->FindKeyOfType("left", base::Value::Type::INTEGER);
    base::Value* right =
        bounds->FindKeyOfType("right", base::Value::Type::INTEGER);
    base::Value* top = bounds->FindKeyOfType("top", base::Value::Type::INTEGER);
    base::Value* bottom =
        bounds->FindKeyOfType("bottom", base::Value::Type::INTEGER);
    if (!left || !right || !top || !bottom) {
      BuildError(id->GetInt(), "Invalid bounds", &message);
      client->DispatchProtocolMessage(agent_host, message);
      return true;
    }

    gfx::Rect rect(left->GetInt(), top->GetInt(),
                   right->GetInt() - left->GetInt(),
                   bottom->GetInt() - top->GetInt());
    shell->SetBounds(rect);

    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetKey(kIdParam, base::Value(id->GetInt()));

    base::Value result(base::Value::Type::DICTIONARY);
    dict.SetKey(kResultParam, std::move(result));

    base::JSONWriter::Write(dict, &message);

    client->DispatchProtocolMessage(agent_host, message);
    return true;
  }
  return false;
}

}  // namespace content
