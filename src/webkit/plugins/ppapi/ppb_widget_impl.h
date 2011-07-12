// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_PLUGINS_PPAPI_PPB_WIDGET_IMPL_H_
#define WEBKIT_PLUGINS_PPAPI_PPB_WIDGET_IMPL_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/thunk/ppb_widget_api.h"
#include "webkit/plugins/ppapi/resource.h"

struct PPB_Widget_Dev;
struct PP_InputEvent;

namespace gfx {
class Rect;
}

namespace webkit {
namespace ppapi {

class PPB_ImageData_Impl;
class PluginInstance;

class PPB_Widget_Impl : public Resource,
                        public ::ppapi::thunk::PPB_Widget_API {
 public:
  explicit PPB_Widget_Impl(PluginInstance* instance);
  virtual ~PPB_Widget_Impl();

  // ResourceObjectBase overrides.
  virtual ::ppapi::thunk::PPB_Widget_API* AsPPB_Widget_API() OVERRIDE;

  // PPB_WidgetAPI implementation.
  virtual PP_Bool Paint(const PP_Rect* rect, PP_Resource ) OVERRIDE;
  virtual PP_Bool HandleEvent(const PP_InputEvent* event) = 0;
  virtual PP_Bool GetLocation(PP_Rect* location) OVERRIDE;
  virtual void SetLocation(const PP_Rect* location) OVERRIDE;

  // Notifies the plugin instance that the given rect needs to be repainted.
  void Invalidate(const PP_Rect* dirty);

 protected:
  virtual PP_Bool PaintInternal(const gfx::Rect& rect,
                                PPB_ImageData_Impl* image) = 0;
  virtual void SetLocationInternal(const PP_Rect* location) = 0;

  PP_Rect location() const { return location_; }

 private:
  PP_Rect location_;

  DISALLOW_COPY_AND_ASSIGN(PPB_Widget_Impl);
};

}  // namespace ppapi
}  // namespace webkit

#endif  // WEBKIT_PLUGINS_PPAPI_PPB_WIDGET_IMPL_H_
