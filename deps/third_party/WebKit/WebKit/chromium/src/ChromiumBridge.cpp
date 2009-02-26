/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ChromiumBridge.h"

#include "WebClipboard.h"
#include "WebImage.h"
#include "WebKitClient.h"
#include "WebKitPrivate.h"
#include "WebMimeRegistry.h"
#include "WebString.h"
#include "WebURL.h"

#include "KURL.h"
#include "NativeImageSkia.h"
#include "NotImplemented.h"
#include <wtf/Assertions.h>

// We are part of the WebKit implementation.
using namespace WebKit;

namespace WebCore {

//-----------------------------------------------------------------------------
// Clipboard

COMPILE_ASSERT(
    int(PasteboardPrivate::HTMLFormat) == int(WebClipboard::FormatHTML),
    FormatHTML);
COMPILE_ASSERT(
    int(PasteboardPrivate::BookmarkFormat) == int(WebClipboard::FormatBookmark),
    FormatBookmark);
COMPILE_ASSERT(
    int(PasteboardPrivate::WebSmartPasteFormat) == int(WebClipboard::FormatSmartPaste),
    FormatSmartPaste);

bool ChromiumBridge::clipboardIsFormatAvailable(
    PasteboardPrivate::ClipboardFormat format)
{
    return webKitClient()->clipboard()->isFormatAvailable(
        static_cast<WebClipboard::Format>(format));
}

String ChromiumBridge::clipboardReadPlainText()
{
    return webKitClient()->clipboard()->readPlainText();
}

void ChromiumBridge::clipboardReadHTML(String* htmlText, KURL* sourceURL)
{
    WebURL url;
    *htmlText = webKitClient()->clipboard()->readHTML(&url);
    *sourceURL = url;
}

void ChromiumBridge::clipboardWriteSelection(const String& htmlText,
                                             const KURL& sourceURL,
                                             const String& plainText,
                                             bool writeSmartPaste)
{
    webKitClient()->clipboard()->writeHTML(
        htmlText, sourceURL, plainText, writeSmartPaste);
}

void ChromiumBridge::clipboardWriteURL(const KURL& url, const String& title)
{
    webKitClient()->clipboard()->writeURL(url, title);
}

void ChromiumBridge::clipboardWriteImage(const NativeImageSkia* image,
                                         const KURL& sourceURL,
                                         const String& title)
{
#if WEBKIT_USING(SKIA)
    webKitClient()->clipboard()->writeImage(
        WebImage(*image), sourceURL, title);
#else
    // FIXME clipboardWriteImage probably shouldn't take a NativeImageSkia
    notImplemented();
#endif
}

//-----------------------------------------------------------------------------
// MIME

bool ChromiumBridge::isSupportedImageMIMEType(const char* mimeType) {
    return webKitClient()->mimeRegistry()->supportsImageMIMEType(String(mimeType));
}

bool ChromiumBridge::isSupportedJavascriptMIMEType(const char* mimeType) {
    return webKitClient()->mimeRegistry()->supportsJavaScriptMIMEType(String(mimeType));
}

bool ChromiumBridge::isSupportedNonImageMIMEType(const char* mimeType) {
    return webKitClient()->mimeRegistry()->supportsNonImageMIMEType(String(mimeType));
}

bool ChromiumBridge::matchesMIMEType(const String& pattern,
                                     const String& type) {
    // FIXME: This method should be removed from ChromiumBridge
    ASSERT_NOT_REACHED();
    return false;
}

String ChromiumBridge::mimeTypeForExtension(const String& extension) {
    return webKitClient()->mimeRegistry()->mimeTypeForExtension(extension);
}

String ChromiumBridge::mimeTypeFromFile(const String& path) {
    return webKitClient()->mimeRegistry()->mimeTypeFromFile(path);
}

String ChromiumBridge::preferredExtensionForMIMEType(const String& mimeType) {
    return webKitClient()->mimeRegistry()->preferredExtensionForMIMEType(mimeType);
}

} // namespace WebCore
