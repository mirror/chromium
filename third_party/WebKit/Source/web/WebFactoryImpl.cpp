// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web/WebFactoryImpl.h"
#include "modules/audio_output_devices/HTMLMediaElementAudioOutputDevice.h"
#include "modules/encryptedmedia/HTMLMediaElementEncryptedMedia.h"
#include "modules/remoteplayback/HTMLMediaElementRemotePlayback.h"
#include "modules/remoteplayback/RemotePlayback.h"
#include "modules/serviceworkers/ServiceWorkerLinkResource.h"
#include "public/web/WebFrameClient.h"
#include "web/ChromeClientImpl.h"
#include "web/WebLocalFrameImpl.h"
#include "web/WebViewImpl.h"

namespace blink {

void WebFactoryImpl::Initialize() {
  WebFactory::SetInstance(*(new WebFactoryImpl()));
}

ChromeClient* WebFactoryImpl::CreateChromeClient(WebViewBase* view) const {
  return ChromeClientImpl::Create(view);
}

WebViewBase* WebFactoryImpl::CreateWebViewBase(
    WebViewClient* client,
    WebPageVisibilityState state) const {
  return WebViewImpl::Create(client, state);
}

WebLocalFrameBase* WebFactoryImpl::CreateMainWebLocalFrameBase(
    WebView* web_view,
    WebFrameClient* client,
    InterfaceRegistry* registry) const {
  return WebLocalFrameImpl::CreateMainFrame(web_view, client, registry, nullptr,
                                            g_empty_atom,
                                            WebSandboxFlags::kNone);
}

WebLocalFrameBase* WebFactoryImpl::CreateWebLocalFrameBase(
    WebTreeScopeType type,
    WebFrameClient* client,
    InterfaceRegistry* registry,
    WebFrame* opener) const {
  return WebLocalFrameImpl::Create(type, client, registry, opener);
}

std::unique_ptr<WebMediaPlayer> WebFactoryImpl::CreateWebMediaPlayer(
    HTMLMediaElement& html_media_element,
    const WebMediaPlayerSource& source,
    WebMediaPlayerClient* client) {
  WebLocalFrameBase* web_frame =
      WebLocalFrameBase::FromFrame(html_media_element.GetDocument().GetFrame());

  if (!web_frame || !web_frame->Client())
    return nullptr;

  HTMLMediaElementEncryptedMedia& encrypted_media =
      HTMLMediaElementEncryptedMedia::From(html_media_element);
  WebString sink_id(
      HTMLMediaElementAudioOutputDevice::sinkId(html_media_element));
  return WTF::WrapUnique(web_frame->Client()->CreateMediaPlayer(
      source, client, &encrypted_media,
      encrypted_media.ContentDecryptionModule(), sink_id));
}

WebRemotePlaybackClient* WebFactoryImpl::CreateWebRemotePlaybackClient(
    HTMLMediaElement& html_media_element) {
  return HTMLMediaElementRemotePlayback::remote(html_media_element);
}

LinkResource* WebFactoryImpl::CreateServiceWorkerLinkResource(
    HTMLLinkElement* owner) {
  return ServiceWorkerLinkResource::Create(owner);
}

}  // namespace blink
