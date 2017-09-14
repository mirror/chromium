// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebMimeHandlerViewManager_h
#define WebMimeHandlerViewManager_h

#include "WebImeTextSpan.h"
#include "WebWidget.h"
#include "public/platform/WebTextInputInfo.h"

namespace v8 {
template <class T>
class Local;
class Isolate;
class Object;
}  // namespace v8

namespace blink {

class WebFrame;
class WebLocalFrame;
class WebString;
class WebURL;

// WebMimeHandlerViewManager coordinates the embedded mime-types which are
// rendered inside a content frame. There is at most on instance of it per
// WebLocalFrame and is owned by the WebLocalFrame itself.
class BLINK_EXPORT WebMimeHandlerViewManager {
 public:
  using HandlerId = int32_t;

  // An invalid frame controller Id.
  static const HandlerId kHandlerIdNone;
  static const HandlerId kFirstHandlerId;

  virtual ~WebMimeHandlerViewManager();

  // Creates a new external handler to render the provided resource and given
  // mime type. It will return a valid HandlerId if the resource can be handled.
  // Upon failure, the returned value if kHandlerIdNone.
  virtual HandlerId CreateHandler(const WebURL& resource_url,
                                  const WebString& mime_type) = 0;

  // Returns handler URL associated with the provided ID. Returns empty URL
  // for unknown and invalid |id| values.
  virtual WebURL GetUrl(HandlerId) const = 0;

  // Returns an scriptable object for the given ID. The scriptable object can
  // be used to implement custom API.
  virtual v8::Local<v8::Object> V8ScriptableObject(HandlerId, v8::Isolate*) = 0;

  // Called when the handler lives in a PluginDocuemnt.
  virtual void DidReceiveDataInPluginDocument(const char* data,
                                              size_t data_length) = 0;

  // Returns a nested WebFrame (under |frame|) associated with the passed ID.
  WebFrame* GetContentFrameFromId(HandlerId);

  // The frame which owns the WebMimeHandlerViewManager.
  WebLocalFrame* owner_frame() const { return owner_frame_; }

 protected:
  explicit WebMimeHandlerViewManager(WebLocalFrame*);

  // Tries to retrieve a HanlderId associated with the given frame. If the frame
  // is not owned by a plugin document the return value will be kHanlderIdNone.
  // Otherwise, it will return the associated |external_handler_id_|.
  static HandlerId GetHandlerId(const WebFrame*);

  // Creates the next HandlerId. This value will uniquely identify the handler
  // for an HTMLPlugInElement directly inside |owner_frame_|.
  HandlerId NextHandlerId();

  // Unassigns the given HandlerId from any child frames which is owned by a
  // plugin element with matching HandlerId.
  void UnassignHandlerId(HandlerId);

 private:
  WebLocalFrame* owner_frame_;
  HandlerId next_handler_id_ = 0;
};

}  // namespace blink
#endif  // WebMimeHandlerViewManager_h
