// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_H_
#define CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace url {
class Origin;
}

namespace content {

class RenderFrameHost;
class RenderProcessHost;
class WebInterfaceFilter;
enum class WebContextType;

class WebInterfaceBroker {
 public:
  WebInterfaceBroker() = default;
  virtual ~WebInterfaceBroker() = default;

  static std::unique_ptr<WebInterfaceBroker> Create();

  virtual bool TryBindInterface(WebContextType,
                                const std::string& interface_name,
                                mojo::ScopedMessagePipeHandle* interface_pipe,
                                RenderProcessHost*,
                                const url::Origin&,
                                mojo::ReportBadMessageCallback*) = 0;

  // Try binding an interface request |interface_pipe| for |interface_name|
  // received from |frame|.
  virtual bool TryBindInterface(const std::string& interface_name,
                                mojo::ScopedMessagePipeHandle* interface_pipe,
                                RenderFrameHost* frame,
                                mojo::ReportBadMessageCallback*) = 0;

  template <typename Interface>
  void AddInterface(
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>,
                                   RenderFrameHost*)> frame_callback,
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>,
                                   RenderProcessHost*,
                                   const url::Origin&)> callback,
      std::unique_ptr<WebInterfaceFilter> filter) {
    AddInterfaceImpl(
        Interface::Name_, std::move(filter),
        CreateForwarder<Interface, RenderFrameHost*>(std::move(frame_callback)),
        CreateForwarder<Interface, RenderProcessHost*, const url::Origin&>(
            std::move(callback)));
  }

  template <typename Interface>
  void AddInterface(
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>,
                                   RenderFrameHost*)> frame_callback,
      std::unique_ptr<WebInterfaceFilter> filter) {
    AddInterfaceImpl(
        Interface::Name_, std::move(filter),
        CreateForwarder<Interface, RenderFrameHost*>(std::move(frame_callback)),
        {});
  }

  template <typename Interface>
  void AddInterface(
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>,
                                   RenderProcessHost*,
                                   const url::Origin&)> callback,
      std::unique_ptr<WebInterfaceFilter> filter) {
    AddInterfaceImpl(
        Interface::Name_, std::move(filter), {},
        CreateForwarder<Interface, RenderProcessHost*, const url::Origin&>(
            std::move(callback)));
  }

  template <typename Interface>
  void AddInterface(const char* service_name,
                    std::unique_ptr<WebInterfaceFilter> filter) {
    AddInterfaceImpl(Interface::Name_, std::move(filter), {},
                     CreateServiceRequestForwarder(service_name));
  }

  template <typename Interface>
  void AddWebContentsObserverInterface(
      std::unique_ptr<WebInterfaceFilter> filter) {
    AddWebContentsObserverInterfaceImpl(Interface::Name_,
                                        std::move(filter));
  }

 private:
  virtual void AddInterfaceImpl(
      const std::string& interface_name,
      std::unique_ptr<WebInterfaceFilter>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderFrameHost*)>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderProcessHost*,
                                   const url::Origin&)>) = 0;

  virtual void AddWebContentsObserverInterface(
      const std::string& interface_name,
      std::unique_ptr<WebInterfaceFilter> filter) = 0;

  static base::RepeatingCallback<void(const std::string&,
                                      mojo::ScopedMessagePipeHandle,
                                      RenderProcessHost*,
                                      const url::Origin&)>
  CreateServiceRequestForwarder(const char* service_name);

  template <typename Interface, typename... Args>
  static void ForwardToBinder(
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>, Args...)>
          binder,
      const std::string&,
      mojo::ScopedMessagePipeHandle handle,
      Args... args) {
    binder.Run(mojo::InterfaceRequest<Interface>(std::move(handle)),
               std::forward<Args>(args)...);
  }

  template <typename Interface, typename... Args>
  static base::RepeatingCallback<
      void(const std::string&, mojo::ScopedMessagePipeHandle, Args...)>
  CreateForwarder(
      base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>, Args...)>
          callback) {
    return base::Bind(&WebInterfaceBroker::ForwardToBinder<Interface, Args...>,
                      std::move(callback));
  }

  DISALLOW_COPY_AND_ASSIGN(WebInterfaceBroker);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_H_
