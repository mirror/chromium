// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_DRAGCLIENT_IMPL_H__
#define WEBKIT_GLUE_DRAGCLIENT_IMPL_H__

#include "base/basictypes.h"
#include "base/compiler_specific.h"

#ifdef ENABLE_BACKGROUND_TASK
#include "base/scoped_ptr.h"
#endif  // ENABLE_BACKGROUND_TASK

MSVC_PUSH_WARNING_LEVEL(0);
#include "DragClient.h"
#include "DragActions.h"
MSVC_POP_WARNING();

namespace WebCore {
class ClipBoard;
class DragData;
class IntPoint;
class KURL;
}

class WebViewImpl;

#ifdef ENABLE_BACKGROUND_TASK
class WebCursor;
#endif  // ENABLE_BACKGROUND_TASK

class DragClientImpl : public WebCore::DragClient {
public:
#if ENABLE_BACKGROUND_TASK
  DragClientImpl(WebViewImpl* webview) : webview_(webview), is_bb_drag_(false) {
#else
  DragClientImpl(WebViewImpl* webview) : webview_(webview) {
#endif  // ENABLE_BACKGROUND_TASK
  }
  virtual ~DragClientImpl() {}

  virtual void willPerformDragDestinationAction(WebCore::DragDestinationAction,
                                                WebCore::DragData*);
  virtual void willPerformDragSourceAction(WebCore::DragSourceAction,
                                           const WebCore::IntPoint&,
                                           WebCore::Clipboard*);
  virtual WebCore::DragDestinationAction actionMaskForDrag(WebCore::DragData*);
  virtual WebCore::DragSourceAction dragSourceActionMaskForPoint(
      const WebCore::IntPoint& window_point);
  
  virtual void startDrag(WebCore::DragImageRef drag_image,
                         const WebCore::IntPoint& drag_image_origin,
                         const WebCore::IntPoint& event_pos,
                         WebCore::Clipboard* clipboard,
                         WebCore::Frame* frame,
                         bool is_link_drag = false);
  virtual WebCore::DragImageRef createDragImageForLink(
      WebCore::KURL&, const WebCore::String& label, WebCore::Frame*); 

#ifdef ENABLE_BACKGROUND_TASK
  virtual WebCore::DragImageRef createDragImageForBbElement(
      const WebCore::HTMLBbElement& bb_element);
#endif  // ENABLE_BACKGROUND_TASK
  
  virtual void dragControllerDestroyed();

private:
  DISALLOW_EVIL_CONSTRUCTORS(DragClientImpl);
  WebViewImpl* webview_;
#if ENABLE_BACKGROUND_TASK
  scoped_ptr<WebCursor> drag_cursor_;
  bool is_bb_drag_;
#endif  // ENABLE_BACKGROUND_TASK
};

#endif  // #ifndef WEBKIT_GLUE_DRAGCLIENT_IMPL_H__

