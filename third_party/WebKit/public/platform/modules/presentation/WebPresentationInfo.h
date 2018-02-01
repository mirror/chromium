// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebPresentationInfo_h
#define WebPresentationInfo_h

#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"

namespace blink {

// Although it's not necessary to include public/web/WebLocalFrame.h, it would
// be disallowed.  Therefore I'm not sure if this is reasonable in
// WebPresentationInfo.  I don't see an immediate way around this though.
class WebLocalFrame;

// A simple wrapper around blink::mojom::PresentationInfo. Since the
// Presentation API has not been Onion Soup-ed, there are portions implemented
// in Blink and the embedder. Because it is not possible to use the STL and WTF
// mojo bindings alongside each other, going between Blink and the embedder
// requires this indirection.
struct WebPresentationInfo {
  WebPresentationInfo(const WebURL& url,
                      const WebString& id,
                      WebLocalFrame* web_frame = nullptr)
      : url(url), id(id), web_frame(web_frame) {}
  WebURL url;
  WebString id;
  WebLocalFrame* web_frame;
};

}  // namespace blink

#endif  // WebPresentationInfo_h
