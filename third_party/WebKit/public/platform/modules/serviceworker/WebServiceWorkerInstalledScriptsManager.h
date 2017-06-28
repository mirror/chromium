// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebServiceWorkerInstalledScriptsManager_h
#define WebServiceWorkerInstalledScriptsManager_h

#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebVector.h"

#if INSIDE_BLINK
#include "platform/network/HTTPHeaderMap.h"
#endif

namespace blink {

class WebServiceWorkerInstalledScriptsManager {
 public:
  using DataVector = WebVector<char>;
  class RawScriptData {
   public:
    RawScriptData(WebString encoding,
                  WebVector<DataVector> script_text,
                  WebVector<DataVector> meta_data);
    void AddHeader(const WebString& key, const WebString& value);

#if INSIDE_BLINK
    const WebString& Encoding() const { return encoding_; }
    const WebVector<DataVector>& ScriptText() const { return script_text_; }
    const WebVector<DataVector>& MetaData() const { return meta_data_; }
    const HTTPHeaderMap& Headers() const { return headers_; }

   private:
    WebString encoding_;
    WebVector<DataVector> script_text_;
    WebVector<DataVector> meta_data_;
    HTTPHeaderMap headers_;
#endif
  };

  virtual ~WebServiceWorkerInstalledScriptsManager() {}

  // Called on the main or worker thread.
  virtual bool IsScriptInstalled(const blink::WebURL& script_url) const = 0;
  // Called on the worker thread.
  virtual std::unique_ptr<RawScriptData> GetRawScriptData(
      const WebURL& script_url) = 0;
};

}  // namespace blink

#endif  // WebServiceWorkerInstalledScriptsManager_h
