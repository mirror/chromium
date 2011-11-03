// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/plugin_globals.h"

#include "ppapi/proxy/plugin_dispatcher.h"

namespace ppapi {
namespace proxy {

PluginGlobals* PluginGlobals::plugin_globals_ = NULL;

PluginGlobals::PluginGlobals() : ppapi::PpapiGlobals() {
  DCHECK(!plugin_globals_);
  plugin_globals_ = this;
}

PluginGlobals::~PluginGlobals() {
  DCHECK(plugin_globals_ == this);
  plugin_globals_ = NULL;
}

ResourceTracker* PluginGlobals::GetResourceTracker() {
  return &plugin_resource_tracker_;
}

VarTracker* PluginGlobals::GetVarTracker() {
  return &plugin_var_tracker_;
}

FunctionGroupBase* PluginGlobals::GetFunctionAPI(PP_Instance inst, ApiID id) {
  PluginDispatcher* dispatcher = PluginDispatcher::GetForInstance(inst);
  if (dispatcher)
    return dispatcher->GetFunctionAPI(id);
  return NULL;
}

PP_Module PluginGlobals::GetModuleForInstance(PP_Instance instance) {
  // Currently proxied plugins don't use the PP_Module for anything useful.
  return 0;
}

}  // namespace proxy
}  // namespace ppapi
