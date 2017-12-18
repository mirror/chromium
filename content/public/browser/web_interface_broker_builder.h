// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_BUILDER_H_
#define CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_BUILDER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace url {
class Origin;
}

namespace content {

class RenderFrameHost;
class RenderProcessHost;
class WebInterfaceBroker;
class WebInterfaceFilter;
enum class WebContextType;

class CONTENT_EXPORT WebInterfaceBrokerBuilder {
 public:
  WebInterfaceBrokerBuilder() = default;
  virtual ~WebInterfaceBrokerBuilder() = default;

  using FrameDelegate =
      base::RepeatingCallback<void(const std::string& interface_name,
                                   mojo::ScopedMessagePipeHandle interface_pipe,
                                   RenderFrameHost* frame,
                                   mojo::ReportBadMessageCallback)>;

  using WorkerDelegate =
      base::RepeatingCallback<void(WebContextType,
                                   const std::string& interface_name,
                                   mojo::ScopedMessagePipeHandle interface_pipe,
                                   RenderProcessHost* process_host,
                                   const url::Origin& origin,
                                   mojo::ReportBadMessageCallback)>;

  static std::unique_ptr<WebInterfaceBrokerBuilder> Create(
      WorkerDelegate worker_delegate,
      FrameDelegate frame_delegate);

  virtual std::unique_ptr<WebInterfaceBroker> Build() = 0;

  // A binder of interfaces that requires the RenderFrameHost. A
  // FrameInterfaceBinder will never be tried for requests from a worker.
  template <typename Interface>
  using FrameInterfaceBinder =
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>,
                                   RenderFrameHost*)>;

  // A binder of interfaces that requires the the origin and RenderProcessHost.
  template <typename Interface>
  using OriginInterfaceBinder =
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>,
                                   RenderProcessHost*,
                                   const url::Origin&)>;

  template <typename Interface>
  void AddInterface(std::unique_ptr<WebInterfaceFilter> filter,
                    FrameInterfaceBinder<Interface> frame_binder,
                    OriginInterfaceBinder<Interface> origin_binder);

  template <typename Interface>
  void AddInterface(std::unique_ptr<WebInterfaceFilter> filter,
                    FrameInterfaceBinder<Interface> frame_binder);

  template <typename Interface>
  void AddInterface(std::unique_ptr<WebInterfaceFilter> filter,
                    OriginInterfaceBinder<Interface> origin_binder);

  template <typename Interface>
  void AddInterface(std::unique_ptr<WebInterfaceFilter> filter,
                    const char* service_name);

  template <typename Interface>
  void AddWebContentsObserverInterface(
      std::unique_ptr<WebInterfaceFilter> filter);

 private:
  virtual void AddInterfaceImpl(
      base::StringPiece interface_name,
      std::unique_ptr<WebInterfaceFilter>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderFrameHost*)>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderProcessHost*,
                                   const url::Origin&)>) = 0;

  virtual void AddWebContentsObserverInterfaceImpl(
      base::StringPiece interface_name,
      std::unique_ptr<WebInterfaceFilter> filter) = 0;

  virtual void ForwardInterfaceToService(
      base::StringPiece interface_name,
      std::unique_ptr<WebInterfaceFilter> filter,
      base::StringPiece service_name) = 0;

  template <typename Interface, typename... Args>
  static void ForwardToBinder(
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>, Args...)>
          binder,
      const std::string&,
      mojo::ScopedMessagePipeHandle handle,
      Args... args);

  template <typename Interface, typename... Args>
  static base::RepeatingCallback<
      void(const std::string&, mojo::ScopedMessagePipeHandle, Args...)>
  CreateForwarder(
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>, Args...)>
          callback);

  DISALLOW_COPY_AND_ASSIGN(WebInterfaceBrokerBuilder);
};

template <typename Interface>
void WebInterfaceBrokerBuilder::AddInterface(
    std::unique_ptr<WebInterfaceFilter> filter,
    FrameInterfaceBinder<Interface> frame_binder,
    OriginInterfaceBinder<Interface> origin_binder) {
  DCHECK(frame_binder);
  DCHECK(origin_binder);
  AddInterfaceImpl(
      Interface::Name_, std::move(filter),
      CreateForwarder<Interface, RenderFrameHost*>(std::move(frame_binder)),
      CreateForwarder<Interface, RenderProcessHost*, const url::Origin&>(
          std::move(origin_binder)));
}

template <typename Interface>
void WebInterfaceBrokerBuilder::AddInterface(
    std::unique_ptr<WebInterfaceFilter> filter,
    FrameInterfaceBinder<Interface> frame_binder) {
  DCHECK(frame_binder);
  AddInterfaceImpl(
      Interface::Name_, std::move(filter),
      CreateForwarder<Interface, RenderFrameHost*>(std::move(frame_binder)),
      {});
}

template <typename Interface>
void WebInterfaceBrokerBuilder::AddInterface(
    std::unique_ptr<WebInterfaceFilter> filter,
    OriginInterfaceBinder<Interface> origin_binder) {
  DCHECK(origin_binder);
  AddInterfaceImpl(
      Interface::Name_, std::move(filter), {},
      CreateForwarder<Interface, RenderProcessHost*, const url::Origin&>(
          std::move(origin_binder)));
}

template <typename Interface>
void WebInterfaceBrokerBuilder::AddInterface(
    std::unique_ptr<WebInterfaceFilter> filter,
    const char* service_name) {
  ForwardInterfaceToService(Interface::Name_, std::move(filter), service_name);
}

template <typename Interface>
void WebInterfaceBrokerBuilder::AddWebContentsObserverInterface(
    std::unique_ptr<WebInterfaceFilter> filter) {
  AddWebContentsObserverInterfaceImpl(Interface::Name_, std::move(filter));
}

// static
template <typename Interface, typename... Args>
void WebInterfaceBrokerBuilder::ForwardToBinder(
    base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>, Args...)>
        binder,
    const std::string&,
    mojo::ScopedMessagePipeHandle handle,
    Args... args) {
  binder.Run(mojo::InterfaceRequest<Interface>(std::move(handle)),
             std::forward<Args>(args)...);
}

// static
template <typename Interface, typename... Args>
base::RepeatingCallback<
    void(const std::string&, mojo::ScopedMessagePipeHandle, Args...)>
WebInterfaceBrokerBuilder::CreateForwarder(
    base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>, Args...)>
        callback) {
  return base::BindRepeating(
      &WebInterfaceBrokerBuilder::ForwardToBinder<Interface, Args...>,
      std::move(callback));
}

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_BUILDER_H_
