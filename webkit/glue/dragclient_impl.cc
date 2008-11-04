// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "base/compiler_specific.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include <objidl.h>
#endif

MSVC_PUSH_WARNING_LEVEL(0);
#if defined(OS_WIN)
#include "ClipboardWin.h"
#include "COMPtr.h"
#endif
#include "DragData.h"
#include "Frame.h"
#include "HitTestResult.h"
#include "Image.h"
#include "KURL.h"
MSVC_POP_WARNING();
#undef LOG

#include "webkit/glue/dragclient_impl.h"

#include "base/logging.h"
#include "base/string_util.h"
#include "webkit/glue/context_node_types.h"
#include "webkit/glue/glue_util.h"
#include "webkit/glue/webdropdata.h"
#include "webkit/glue/webview_delegate.h"
#include "webkit/glue/webview_impl.h"

#ifdef ENABLE_BACKGROUND_TASK
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "PlatformContextSkia.h"
#include "base/gfx/platform_canvas_win.h"
#include "webkit/glue/webcursor.h"
#endif  // ENABLE_BACKGROUND_TASK

void DragClientImpl::willPerformDragDestinationAction(
    WebCore::DragDestinationAction,
    WebCore::DragData*) {
  // FIXME
}

void DragClientImpl::willPerformDragSourceAction(
    WebCore::DragSourceAction drag_action,
    const WebCore::IntPoint&,
    WebCore::Clipboard*) {
#ifdef ENABLE_BACKGROUND_TASK
  is_bb_drag_ = (drag_action == WebCore::DragSourceActionBB);
#endif  // ENABLE_BACKGROUND_TASK
}

WebCore::DragDestinationAction DragClientImpl::actionMaskForDrag(
    WebCore::DragData*) {
  return WebCore::DragDestinationActionAny; 
}

WebCore::DragSourceAction DragClientImpl::dragSourceActionMaskForPoint(
    const WebCore::IntPoint& window_point) {
  // We want to handle drag operations for all source types.
  return WebCore::DragSourceActionAny;
}

void DragClientImpl::startDrag(WebCore::DragImageRef drag_image,
                               const WebCore::IntPoint& drag_image_origin,
                               const WebCore::IntPoint& event_pos,
                               WebCore::Clipboard* clipboard,
                               WebCore::Frame* frame,
                               bool is_link_drag) {
  // Add a ref to the frame just in case a load occurs mid-drag.
  RefPtr<WebCore::Frame> frame_protector = frame;

#if defined(OS_WIN)
  COMPtr<IDataObject> data_object(
      static_cast<WebCore::ClipboardWin*>(clipboard)->dataObject());
  DCHECK(data_object.get());
  WebDropData drop_data;
  WebDropData::PopulateWebDropData(data_object.get(), &drop_data);
#elif defined(OS_MACOSX) || defined(OS_LINUX)
  WebDropData drop_data;
#endif

#ifdef ENABLE_BACKGROUND_TASK
  drop_data.is_bb_drag = is_bb_drag_;
  if (is_bb_drag_ && drag_cursor_.get()) {

    drag_cursor_->set_hotspot(
        event_pos.x() - drag_cursor_->hotspot_x(), 
        event_pos.y() - drag_cursor_->hotspot_y());
    drop_data.drag_cursor = *drag_cursor_;  // make a copy
    drag_cursor_.reset();
  }
#endif  // ENABLE_BACKGROUND_TASK

  webview_->StartDragging(drop_data);
}

WebCore::DragImageRef DragClientImpl::createDragImageForLink(
    WebCore::KURL&,
    const WebCore::String& label,
    WebCore::Frame*) {
  // FIXME
  return 0;
}

#ifdef ENABLE_BACKGROUND_TASK
WebCore::DragImageRef DragClientImpl::createDragImageForBbElement(
    const WebCore::HTMLBbElement& bb_element) {
  WebCore::RenderObject* renderer = bb_element.renderer();
  if (!renderer) {
    return 0;
  }

  renderer->layoutIfNeeded();
  int width = renderer->width();
  int height = renderer->height();
  if (width <= 0 || height <= 0) {
    return 0;
  }

  // Get some paintable image to render the element to.
  std::auto_ptr<WebCore::ImageBuffer> image_buffer = 
      WebCore::ImageBuffer::create(WebCore::IntSize(width, height), false);
  WebCore::GraphicsContext* context = image_buffer->context();

  // Fill the background - element subtree may not paint every pixel.
  context->setFillColor(WebCore::Color::white);
  context->drawRect(WebCore::IntRect(0, 0, width, height));

  WebCore::RenderObject::PaintInfo info(
      context, 
      WebCore::IntRect(0, 0, width, height), 
      WebCore::PaintPhaseBlockBackground, 0, 0, 0);

  int offset_x = -(renderer->xPos());
  int offset_y = -(renderer->yPos());

  // Render aspects we might beinterested in.
  // TODO: this is not a complete set of phases, find out if we need more.
  info.phase = WebCore::PaintPhaseBlockBackground;
  renderer->paint(info, offset_x, offset_y);
  info.phase = WebCore::PaintPhaseChildBlockBackgrounds;
  renderer->paint(info, offset_x, offset_y);
  info.phase = WebCore::PaintPhaseFloat;
  renderer->paint(info, offset_x, offset_y);
  info.phase = WebCore::PaintPhaseForeground;
  renderer->paint(info, offset_x, offset_y);
  info.phase = WebCore::PaintPhaseOutline;
  renderer->paint(info, offset_x, offset_y);

  // Get a copy.
  SkBitmap bitmap;
  context->platformContext()->bitmap()->copyTo(&bitmap, 
                                               SkBitmap::kARGB_8888_Config);

  // Fix up text color. In Win version of Chrome, the alpha channel of text
  // ends up being 0 (since it's the GDI call ExtTextOut that is used to render
  // directly into the bitmap. This perhaps needs more thought as things like
  // rounded corners do not work well.
  uint32_t* row_start = bitmap.getAddr32(0, 0);
  for (int y = 0; y < bitmap.height(); ++y) {
    uint32_t* pixel = row_start;
    for (int x = 0; x < bitmap.width(); ++x) {
      *pixel = *pixel | 0xFF000000;
      ++pixel;
    }
    row_start += bitmap.rowBytesAsPixels();
  }

  int x, y;
  renderer->absolutePosition(x, y);
  drag_cursor_.reset(new WebCursor(&bitmap, x, y));

  return 0;
}
#endif  // ENABLE_BACKGROUND_TASK

void DragClientImpl::dragControllerDestroyed() {
  delete this;
}

