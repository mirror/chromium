// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_PLUGINS_PPAPI_PPB_SCROLLBAR_IMPL_H_
#define WEBKIT_PLUGINS_PPAPI_PPB_SCROLLBAR_IMPL_H_

#include <vector>

#include "ppapi/thunk/ppb_scrollbar_api.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebRect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScrollbarClient.h"
#include "ui/gfx/rect.h"
#include "webkit/plugins/ppapi/ppb_widget_impl.h"

namespace webkit {
namespace ppapi {

class PluginInstance;

class PPB_Scrollbar_Impl : public PPB_Widget_Impl,
                           public ::ppapi::thunk::PPB_Scrollbar_API,
                           public WebKit::WebScrollbarClient {
 public:
  PPB_Scrollbar_Impl(PluginInstance* instance, bool vertical);
  virtual ~PPB_Scrollbar_Impl();

  // ResourceObjectBase override.
  virtual PPB_Scrollbar_API* AsPPB_Scrollbar_API() OVERRIDE;

  // Returns a pointer to the interface implementing PPB_Scrollbar_0_3 that is
  // exposed to the plugin. New code should use the thunk system for the new
  // version of this API.
  static const PPB_Scrollbar_0_3_Dev* Get0_3Interface();

  // PPB_Scrollbar_API implementation.
  virtual uint32_t GetThickness() OVERRIDE;
  virtual uint32_t GetValue() OVERRIDE;
  virtual void SetValue(uint32_t value) OVERRIDE;
  virtual void SetDocumentSize(uint32_t size) OVERRIDE;
  virtual void SetTickMarks(const PP_Rect* tick_marks, uint32_t count) OVERRIDE;
  virtual void ScrollBy(PP_ScrollBy_Dev unit, int32_t multiplier) OVERRIDE;

  // PPB_Widget public implementation.
  virtual PP_Bool HandleEvent(const PP_InputEvent* event) OVERRIDE;

 private:
  // PPB_Widget private implementation.
  virtual PP_Bool PaintInternal(const gfx::Rect& rect,
                                PPB_ImageData_Impl* image) OVERRIDE;
  virtual void SetLocationInternal(const PP_Rect* location) OVERRIDE;

  // WebKit::WebScrollbarClient implementation.
  virtual void valueChanged(WebKit::WebScrollbar* scrollbar) OVERRIDE;
  virtual void invalidateScrollbarRect(WebKit::WebScrollbar* scrollbar,
                                       const WebKit::WebRect& rect) OVERRIDE;
  virtual void getTickmarks(
      WebKit::WebScrollbar* scrollbar,
      WebKit::WebVector<WebKit::WebRect>* tick_marks) const OVERRIDE;

  void NotifyInvalidate();

  gfx::Rect dirty_;
  std::vector<WebKit::WebRect> tickmarks_;
  scoped_ptr<WebKit::WebScrollbar> scrollbar_;

  DISALLOW_COPY_AND_ASSIGN(PPB_Scrollbar_Impl);
};

}  // namespace ppapi
}  // namespace webkit

#endif  // WEBKIT_PLUGINS_PPAPI_PPB_SCROLLBAR_IMPL_H_
