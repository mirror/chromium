// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebFactory_h
#define WebFactory_h

#include <memory>

#include "core/CoreExport.h"
#include "public/platform/WebPageVisibilityState.h"
#include "public/web/WebSandboxFlags.h"

namespace blink {

class ChromeClient;
class HTMLLinkElement;
class HTMLMediaElement;
class InterfaceRegistry;
class LinkResource;
class WebFrame;
class WebFrameClient;
class WebLocalFrameBase;
class WebMediaPlayer;
class WebMediaPlayerSource;
class WebMediaPlayerClient;
class WebRemotePlaybackClient;
class WebView;
class WebViewBase;
class WebViewClient;
enum class WebTreeScopeType;

// WebFactory is a temporary class implemented in web/ that allows classes to
// construct interfaces that are being moved out of web/.
// This class will be removed once all implementations are in core/ or modules/.
class CORE_EXPORT WebFactory {
 public:
  static WebFactory& GetInstance();

  virtual ChromeClient* CreateChromeClient(WebViewBase*) const = 0;
  virtual WebViewBase* CreateWebViewBase(WebViewClient*,
                                         WebPageVisibilityState) const = 0;
  virtual WebLocalFrameBase* CreateMainWebLocalFrameBase(
      WebView*,
      WebFrameClient*,
      InterfaceRegistry*) const = 0;
  virtual WebLocalFrameBase* CreateWebLocalFrameBase(
      WebTreeScopeType,
      WebFrameClient*,
      InterfaceRegistry*,
      WebFrame* opener) const = 0;

  // TODO(joelhockey): Cleanup these methods that are purely to create objects
  // of classes defined in modules from core. See crbug.com/734450
  virtual std::unique_ptr<WebMediaPlayer> CreateWebMediaPlayer(
      HTMLMediaElement&,
      const WebMediaPlayerSource&,
      WebMediaPlayerClient*) = 0;
  virtual WebRemotePlaybackClient* CreateWebRemotePlaybackClient(
      HTMLMediaElement&) = 0;
  virtual LinkResource* CreateServiceWorkerLinkResource(
      HTMLLinkElement* owner) = 0;

 protected:
  // Takes ownership of |factory|.
  static void SetInstance(WebFactory&);

 private:
  static WebFactory* factory_instance_;
};

}  // namespace blink

#endif
