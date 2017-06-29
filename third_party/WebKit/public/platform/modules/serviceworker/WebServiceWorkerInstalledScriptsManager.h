// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebServiceWorkerInstalledScriptsManager_h
#define WebServiceWorkerInstalledScriptsManager_h

#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebVector.h"

#if INSIDE_BLINK
#include <memory>
#include "platform/network/HTTPHeaderMap.h"
#endif

namespace blink {

// WebServiceWorkerInstalledScriptsManager provides installed main script and
// imported scripts to the service worker thread. This is a cross-thread class,
// so implementation needs appropriate arbitration mechanism.
class WebServiceWorkerInstalledScriptsManager {
 public:
  using BytesChunk = WebVector<char>;
  // Container of a script. This has encoding of the script text, headers and
  // arrays of raw byte chunks for the script text and the meta data which is
  // stored as the script's code cache.
  class RawScriptData {
   public:
    RawScriptData(WebString encoding,
                  WebVector<BytesChunk> script_text,
                  WebVector<BytesChunk> meta_data);
    void AddHeader(const WebString& key, const WebString& value);

#if INSIDE_BLINK
    const WebString& Encoding() const { return encoding_; }
    const WebVector<BytesChunk>& ScriptText() const { return script_text_; }
    const WebVector<BytesChunk>& MetaData() const { return meta_data_; }
    std::unique_ptr<CrossThreadHTTPHeaderMapData> DrainHeaders() {
      return std::move(headers_);
    }

   private:
    WebString encoding_;
    WebVector<BytesChunk> script_text_;
    WebVector<BytesChunk> meta_data_;
    std::unique_ptr<CrossThreadHTTPHeaderMapData> headers_;
#endif
  };

  virtual ~WebServiceWorkerInstalledScriptsManager() = default;

  // Called on the main or worker thread.
  virtual bool IsScriptInstalled(const blink::WebURL& script_url) const = 0;
  // Called on the worker thread.
  virtual std::unique_ptr<RawScriptData> GetRawScriptData(
      const WebURL& script_url) = 0;
};

}  // namespace blink

#endif  // WebServiceWorkerInstalledScriptsManager_h
