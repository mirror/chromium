// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PPP_INSTANCE_PROXY_H_
#define PPAPI_PROXY_PPP_INSTANCE_PROXY_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/proxy/host_resource.h"
#include "ppapi/proxy/interface_proxy.h"
#include "ppapi/shared_impl/ppp_instance_combined.h"

struct PP_InputEvent;
struct PP_Rect;

namespace pp {
namespace proxy {

class SerializedVarReturnValue;

class PPP_Instance_Proxy : public InterfaceProxy {
 public:
  template <class PPP_Instance_Type>
  PPP_Instance_Proxy(Dispatcher* dispatcher,
                     const PPP_Instance_Type* target_interface)
      : InterfaceProxy(dispatcher, static_cast<const void*>(target_interface)),
        combined_interface_(
            new ::ppapi::PPP_Instance_Combined(*target_interface)) {
  }
  virtual ~PPP_Instance_Proxy();

  // Return the info for the 0.4 version of the interface.
  static const Info* GetInfo0_4();

  // Return the info for the 0.5 (latest, canonical) version of the interface.
  static const Info* GetInfo0_5();

  ::ppapi::PPP_Instance_Combined* ppp_instance_target() const {
    return combined_interface_.get();
  }

  // InterfaceProxy implementation.
  virtual bool OnMessageReceived(const IPC::Message& msg);

 private:
  // Message handlers.
  void OnMsgDidCreate(PP_Instance instance,
                      const std::vector<std::string>& argn,
                      const std::vector<std::string>& argv,
                      PP_Bool* result);
  void OnMsgDidDestroy(PP_Instance instance);
  void OnMsgDidChangeView(PP_Instance instance,
                          const PP_Rect& position,
                          const PP_Rect& clip,
                          PP_Bool fullscreen);
  void OnMsgDidChangeFocus(PP_Instance instance, PP_Bool has_focus);
  void OnMsgHandleInputEvent(PP_Instance instance,
                             const PP_InputEvent& event,
                             PP_Bool* result);
  void OnMsgHandleDocumentLoad(PP_Instance instance,
                               const HostResource& url_loader,
                               PP_Bool* result);
  void OnMsgGetInstanceObject(PP_Instance instance,
                              SerializedVarReturnValue result);
  scoped_ptr< ::ppapi::PPP_Instance_Combined> combined_interface_;
};

}  // namespace proxy
}  // namespace pp

#endif  // PPAPI_PROXY_PPP_INSTANCE_PROXY_H_
