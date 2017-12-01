// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_INTERFACES_INTERFACE_BROKER_H_
#define CONTENT_PUBLIC_BROWSER_INTERFACES_INTERFACE_BROKER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "content/public/browser/interfaces/interface_filter.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace url {
class Origin;
}

namespace content {

class RenderFrameHost;
class RenderProcessHost;

class InterfaceBroker {
 public:
  InterfaceBroker() = default;
  virtual ~InterfaceBroker() = default;

  static std::unique_ptr<InterfaceBroker> Create();

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
  void AddInterface(base::Callback<void(mojo::InterfaceRequest<Interface>,
                                        RenderFrameHost*)> frame_callback,
                    base::Callback<void(mojo::InterfaceRequest<Interface>,
                                        RenderProcessHost*,
                                        const url::Origin&)> callback,
                    InterfaceConfiguration configuration) {
    AddInterfaceImpl(
        Interface::Name_, std::move(configuration),
        CreateForwarder<Interface, RenderFrameHost*>(std::move(frame_callback)),
        CreateForwarder<Interface, RenderProcessHost*, const url::Origin&>(
            std::move(callback)));
  }

  template <typename Interface>
  void AddInterface(base::Callback<void(mojo::InterfaceRequest<Interface>,
                                        RenderFrameHost*)> frame_callback,
                    InterfaceConfiguration configuration) {
    AddInterfaceImpl(
        Interface::Name_, std::move(configuration),
        CreateForwarder<Interface, RenderFrameHost*>(std::move(frame_callback)),
        {});
  }

  template <typename Interface>
  void AddInterface(base::Callback<void(mojo::InterfaceRequest<Interface>,
                                        RenderProcessHost*,
                                        const url::Origin&)> callback,
                    InterfaceConfiguration configuration) {
    AddInterfaceImpl(
        Interface::Name_, std::move(configuration), {},
        CreateForwarder<Interface, RenderProcessHost*, const url::Origin&>(
            std::move(callback)));
  }

  template <typename Interface>
  void AddInterface(const char* service_name,
                    InterfaceConfiguration configuration) {
    AddInterfaceImpl(Interface::Name_, std::move(configuration), {},
                     CreateServiceRequestForwarder(service_name));
  }

  template <typename Interface>
  void AddWebContentsObserverInterface(InterfaceConfiguration configuration) {
    AddWebContentsObserverInterfaceImpl(Interface::Name_,
                                        std::move(configuration));
  }

 private:
  virtual void AddInterfaceImpl(
      const std::string& interface_name,
      InterfaceConfiguration,
      base::Callback<void(const std::string&,
                          mojo::ScopedMessagePipeHandle,
                          RenderFrameHost*)>,
      base::Callback<void(const std::string&,
                          mojo::ScopedMessagePipeHandle,
                          RenderProcessHost*,
                          const url::Origin&)>) = 0;

  virtual void AddWebContentsObserverInterface(
      const std::string& interface_name,
      InterfaceConfiguration configuration) = 0;

  static base::Callback<void(const std::string&,
                             mojo::ScopedMessagePipeHandle,
                             RenderProcessHost*,
                             const url::Origin&)>
  CreateServiceRequestForwarder(const char* service_name);

  template <typename Interface, typename... Args>
  static void ForwardToBinder(
      base::Callback<void(mojo::InterfaceRequest<Interface>, Args...)> binder,
      const std::string&,
      mojo::ScopedMessagePipeHandle handle,
      Args... args) {
    binder.Run(mojo::InterfaceRequest<Interface>(std::move(handle)),
               std::forward<Args>(args)...);
  }

  template <typename Interface, typename... Args>
  static base::Callback<
      void(const std::string&, mojo::ScopedMessagePipeHandle, Args...)>
  CreateForwarder(base::Callback<void(mojo::InterfaceRequest<Interface>,
                                      Args...)> callback) {
    return base::Bind(&InterfaceBroker::ForwardToBinder<Interface, Args...>,
                      std::move(callback));
  }

  DISALLOW_COPY_AND_ASSIGN(InterfaceBroker);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_INTERFACES_INTERFACE_BROKER_H_
